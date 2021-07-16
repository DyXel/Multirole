#include "Replay.hpp"

#include <cassert>
#include <cstring>

#include "Config.hpp"
#include "Constants.hpp"
#include "StringUtils.hpp"
#include "LZMA/LzmaEnc.h"
#include "LZMA/Alloc.h" // g_Alloc

namespace YGOPro
{

constexpr auto ENCODED_SERVER_VERSION = [](const auto& v) constexpr -> uint32_t
{
	return  (v.client.major & 0xFF)         |
	       ((v.client.minor & 0xFF) << 8 )  |
	       ((v.core.major   & 0xFF) << 16)  |
	       ((v.core.minor   & 0xFF) << 24);
}(SERVER_VERSION);

#include "../../Write.inl"

enum ReplayTypes
{
	REPLAY_YRP1 = 0x31707279,
	REPLAY_YRPX = 0x58707279
};

enum ReplayFlags
{
	REPLAY_COMPRESSED     = 0x1,
	REPLAY_TAG            = 0x2,
	REPLAY_DECODED        = 0x4,
	REPLAY_SINGLE_MODE    = 0x8,
	REPLAY_LUA64          = 0x10,
	REPLAY_NEWREPLAY      = 0x20,
	REPLAY_HAND_TEST      = 0x40,
	REPLAY_DIRECT_SEED    = 0x80,
	REPLAY_64BIT_DUELFLAG = 0x100,
};

struct ReplayHeader
{
	uint32_t type; // See ReplayTypes.
	uint32_t version; // Unused atm, should be set to YGOPro::ClientVersion.
	uint32_t flags; // See ReplayFlags.
	uint32_t seed; // Unix timestamp for YRPX. Core duel seed for YRP.
	uint32_t size; // Uncompressed size of whatever is after this header.
	uint32_t hash; // Unused.
	uint8_t props[8]; // Used for LZMA compression (check their apis).
};

// ***** YRPX Binary format *****
// ReplayHeader
// team0Count [uint32_t]
// team0Names [20 char16_t * team0Count]
// team1Count [uint32_t]
// team1Names [20 char16_t * team1Count]
// duelFlags [uint64_t]
// Core messages (repeat for number of messages):
// 	msgType [uint8_t]
// 	length [uint32_t]
// 	data [uint8_t * length]

// ***** YRP Binary format *****
// ReplayHeader
// team0Count [uint32_t]
// team0Names [20 char16_t * team0Count]
// team1Count [uint32_t]
// team1Names [20 char16_t * team1Count]
// startingLP [uint32_t]
// startingDrawCount [uint32_t]
// drawCountPerTurn [uint32_t]
// duelFlags [uint64_t]
// Deck & Extra Decks (repeat for each duelist):
// 	deckCount [uint32_t]
// 	cards [uint32_t * deckCount]
// 	extraCount [uint32_t]
// 	cards [uint32_t * extraCount]
// Extra cards:
// 	count [uint32_t]
// 	cards [uint32_t * count]
// Core responses (repeat for number of responses):
// 	length [uint8_t]
// 	data [uint8_t * length]

Replay::Replay(
	uint32_t unixTimestamp,
	uint32_t seed,
	const HostInfo& info,
	const CodeVector& extraCards) noexcept
	:
	unixTimestamp(unixTimestamp),
	seed(seed),
	startingLP(info.startingLP),
	startingDrawCount(info.startingDrawCount),
	drawCountPerTurn(info.drawCountPerTurn),
	duelFlags(HostInfo::OrDuelFlags(info.duelFlagsHigh, info.duelFlagsLow)),
	extraCards(extraCards)
{}

const std::vector<uint8_t>& Replay::Bytes() const noexcept
{
	return bytes;
}

void Replay::AddDuelist(uint8_t team, uint8_t pos, Duelist&& duelist) noexcept
{
	duelists[team].insert_or_assign(pos, duelist);
}

void Replay::RecordMsg(const std::vector<uint8_t>& msg) noexcept
{
	// Filter out some useless messages.
	switch(msg[0U])
	{
		case MSG_HINT:
		{
			switch(msg[1U])
			{
				// Do not record player specific hints.
				case 1U: case 2U:
				case 3U: case 5U:
					return;
			}
			break;
		}
		case MSG_SELECT_BATTLECMD:
		case MSG_SELECT_IDLECMD:
		case MSG_SELECT_EFFECTYN:
		case MSG_SELECT_YESNO:
		case MSG_SELECT_OPTION:
		case MSG_SELECT_CHAIN:
		case MSG_SELECT_PLACE:
		case MSG_SELECT_DISFIELD:
		case MSG_SELECT_POSITION:
		case MSG_SELECT_COUNTER:
		case MSG_SELECT_SUM:
		case MSG_SORT_CARD:
		case MSG_SORT_CHAIN:
		case MSG_ROCK_PAPER_SCISSORS:
		case MSG_ANNOUNCE_RACE:
		case MSG_ANNOUNCE_ATTRIB:
		case MSG_ANNOUNCE_CARD:
		case MSG_ANNOUNCE_NUMBER:
		case MSG_SELECT_CARD:
		case MSG_SELECT_TRIBUTE:
		case MSG_SELECT_UNSELECT_CARD:
			return;
	}
	messages.emplace_back(msg);
}

void Replay::RecordResponse(const std::vector<uint8_t>& response) noexcept
{
	responses.emplace_back(response);
}

void Replay::PopBackResponse() noexcept
{
	responses.pop_back();
}

void Replay::Serialize() noexcept
{
	auto YRPXPastHeaderSize = [&]() -> std::size_t
	{
		std::size_t size =
			8U + // team0Count<4> + team1Count<4>
			8U;  // duelFlags<8>
		// Size occupied by each duelist
		size += 40U * (duelists[0U].size() + duelists[1U].size());
		// Size occupied by all core messages.
		size += [&]() -> std::size_t
		{
			std::size_t v{};
			for(const auto& msg : messages)
				v += msg.size() + 4U; // length.
			return v;
		}();
		return size;
	};
	auto YRPPastHeaderSize = [&]() -> std::size_t
	{
		std::size_t size =
			8U + // team0Count<4> + team1Count<4>
			8U + // startingLP<4> + startingDrawCount<4>
			12U; // drawCountPerTurn<4> + duelFlags<8>
		// Size occupied by each duelist and their decks.
		for(const auto& m : duelists)
		{
			for(const auto& d : m)
			{
				size +=
					40U + // name<2 * 20>
					8U;   // deckCount<4> + extraCount<4>
				size += d.second.main.size() * 4U;
				size += d.second.extra.size() * 4U;
			}
		}
		// Size occupied by extra cards.
		size += 4U + extraCards.size() * 4U;
		// Size occupied by all player responses
		for(const auto& r : responses)
			size += r.size() + 1U; // length
		return size;
	};
	// Write duelists count and their names.
	auto WriteDuelists = [&](uint8_t*& ptr)
	{
		for(std::size_t team = 0U; team < duelists.size(); team++)
		{
			Write(ptr, static_cast<uint32_t>(duelists[team].size()));
			for(const auto& d : duelists[team])
			{
				const auto str16 = UTF8ToUTF16(d.second.name);
				std::memcpy(ptr, str16.data(), UTF16ByteCount(str16));
				ptr += 40U; // NOTE: Assuming all bytes were initialized to 0
			}
		}
	};
	// YRP replay is appended as a CORE message onto the YRPX messages list,
	// for that reason, we serialize it first and as last step we serialize
	// the whole YRPX past-the-header data.
	[&](std::vector<uint8_t>& vec)
	{
		vec.resize(1U + sizeof(ReplayHeader) + YRPPastHeaderSize());
		uint8_t* ptr = vec.data();
		auto WriteCodeVector = [&ptr](const std::vector<uint32_t>& vec)
		{
			Write(ptr, static_cast<uint32_t>(vec.size()));
			for(const auto& code : vec)
				Write<uint32_t>(ptr, code);
		};
		// NOLINTNEXTLINE: Message type, Called OLD_REPLAY_FORMAT in common.h.
		Write<uint8_t>(ptr, 231U);
		// Replay header for YRP replay format.
		Write(ptr, ReplayHeader
		{
			REPLAY_YRP1,
			ENCODED_SERVER_VERSION,
			REPLAY_LUA64 | REPLAY_NEWREPLAY | REPLAY_DIRECT_SEED | REPLAY_64BIT_DUELFLAG,
			seed,
			static_cast<uint32_t>(YRPPastHeaderSize()),
			0U,
			{}
		});
		// Duelists count and their names.
		WriteDuelists(ptr);
		// Core flags.
		Write<uint32_t>(ptr, startingLP);
		Write<uint32_t>(ptr, startingDrawCount);
		Write<uint32_t>(ptr, drawCountPerTurn);
		Write<uint64_t>(ptr, duelFlags);
		// Decks & Extra Decks.
		for(const auto& m : duelists)
		{
			for(const auto& d : m)
			{
				WriteCodeVector(d.second.main);
				WriteCodeVector(d.second.extra);
			}
		}
		// Extra Cards.
		WriteCodeVector(extraCards);
		// Core responses.
		for(const auto& r : responses)
		{
			Write(ptr, static_cast<uint8_t>(r.size()));
			std::memcpy(ptr, r.data(), r.size());
			ptr += r.size();
		}
		// Number of bytes written shall equal vec.size().
		assert(static_cast<std::size_t>(ptr - vec.data()) == vec.size());
	}(messages.emplace_back());
	// Write past-the-header data for YRPX replay format.
	auto pthData = [&]() -> std::vector<uint8_t>
	{
		std::vector<uint8_t> vec(YRPXPastHeaderSize());
		uint8_t* ptr = vec.data();
		WriteDuelists(ptr);
		// Duel flags.
		Write<uint64_t>(ptr, duelFlags);
		// Core messages.
		for(const auto& msg : messages)
		{
			const std::size_t bodyLength = msg.size() - 1U;
			Write<uint8_t>(ptr, msg[0U]);
			Write(ptr, static_cast<uint32_t>(bodyLength));
			std::memcpy(ptr, msg.data() + 1U, bodyLength);
			ptr += bodyLength;
		}
		// Number of bytes written shall equal vec.size().
		assert(static_cast<std::size_t>(ptr - vec.data()) == vec.size());
		return vec;
	}();
	// Replay header for YRPX replay format.
	ReplayHeader header
	{
		REPLAY_YRPX,
		ENCODED_SERVER_VERSION,
		REPLAY_LUA64 | REPLAY_NEWREPLAY | REPLAY_64BIT_DUELFLAG,
		unixTimestamp,
		static_cast<uint32_t>(pthData.size()),
		0U,
		{}
	};
	// Compress past-the-header data.
	std::vector<uint8_t> compData(pthData.size() * 2U);
	CLzmaEncProps props;
	LzmaEncProps_Init(&props);
	props.numThreads = 1; // NOLINT: No multithreading.
	SizeT destLen = compData.size();
	SizeT outPropSize = LZMA_PROPS_SIZE;
	LzmaEncode
	(
		compData.data(),
		&destLen,
		pthData.data(),
		pthData.size(),
		&props,
		header.props,
		&outPropSize,
		0,
		nullptr,
		&g_Alloc,
		&g_Alloc
	);
	compData.resize(destLen);
	header.flags |= REPLAY_COMPRESSED;
	pthData = std::move(compData);
	// Write final binary replay.
	bytes.resize(sizeof(ReplayHeader) + pthData.size());
	uint8_t* ptr = bytes.data();
	Write<ReplayHeader>(ptr, header);
	std::memcpy(ptr, pthData.data(), pthData.size());
	// Remove message that was appended for serializing purposes.
	messages.pop_back();
}

} // namespace YGOPro
