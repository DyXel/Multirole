#include "Replay.hpp"

#include <cassert>
#include <cstring>

#include "Constants.hpp"
#include "StringUtils.hpp"

namespace YGOPro
{

#include "Write.inl"

enum ReplayTypes
{
	REPLAY_YRP1 = 0x31707279,
	REPLAY_YRPX = 0x58707279
};

enum ReplayFlags
{
	REPLAY_COMPRESSED  = 0x1,
	REPLAY_TAG         = 0x2,
	REPLAY_DECODED     = 0x4,
	REPLAY_SINGLE_MODE = 0x8,
	REPLAY_LUA64       = 0x10,
	REPLAY_NEWREPLAY   = 0x20,
	REPLAY_HAND_TEST   = 0x40,
};

struct ReplayHeader
{
	uint32_t type; // See ReplayTypes
	uint32_t version; // Unused atm, should be set to YGOPro::ClientVersion
	uint32_t flags; // See ReplayFlags
	uint32_t seed; // Unix timestamp for YRPX. Core duel seed for YRP
	uint32_t size; // Uncompressed size of whatever is after this header
	uint32_t hash; // Unused
	uint8_t props[8]; // Used for LZMA compression (check their apis)
};

Replay::Replay(uint32_t seed, const HostInfo& info, const CodeVector& extraCards) :
	seed(seed),
	startingLP(info.startingLP),
	startingDrawCount(info.startingDrawCount),
	drawCountPerTurn(info.drawCountPerTurn),
	duelFlags(info.duelFlags),
	extraCards(extraCards)
{}

const std::vector<uint8_t>& Replay::Bytes() const
{
	return bytes;
}

void Replay::AddDuelist(uint8_t team, Duelist&& duelist)
{
	duelists[team].emplace_back(duelist);
}

void Replay::RecordMsg(const std::vector<uint8_t>& msg)
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

void Replay::RecordResponse(const std::vector<uint8_t>& response)
{
	responses.emplace_back(response);
}

void Replay::Serialize()
{
	auto Header1Size = [&]() -> uint32_t
	{
		uint32_t size =
			8U + // team0Count<4> + team1Count<4>
			4U;  // duelFlags<4>
		// Size occupied by each duelist
		size += 40U * (duelists[0U].size() + duelists[1U].size());
		// Size occupied by all core messages
		size += [&]() -> uint32_t
		{
			uint32_t v{};
			for(const auto& msg : messages)
				v += static_cast<uint32_t>(msg.size()) + 4U; // length
			return v;
		}();
		return size;
	};
	// Replay header for new replay format
	const ReplayHeader header1
	{
		REPLAY_YRPX,
		0U, // TODO
		REPLAY_LUA64 | REPLAY_NEWREPLAY,
		0U, // TODO
		Header1Size(),
		0U,
		{}
	};
	// Write past-the-header data for new replay format
	const auto pthData1 = [&]() -> std::vector<uint8_t>
	{
		std::vector<uint8_t> vec(header1.size);
		uint8_t* ptr = vec.data();
		// Duelist number and their names
		for(std::size_t team = 0U; team < duelists.size(); team++)
		{
			Write(ptr, static_cast<uint32_t>(duelists[team].size()));
			for(const auto& d : duelists[team])
			{
				const auto str16 = UTF8ToUTF16(d.name);
				std::memcpy(ptr, str16.data(), UTF16ByteCount(str16));
				ptr += 40U; // NOTE: Assuming all bytes were initialized to 0
			}
		}
		// Duel flags
		Write<uint32_t>(ptr, duelFlags);
		// Core messages
		for(const auto& msg : messages)
		{
			const std::size_t bodyLength = msg.size() - 1U;
			Write<uint8_t>(ptr, msg[0U]);
			Write(ptr, static_cast<uint32_t>(bodyLength));
			std::memcpy(ptr, msg.data() + 1U, bodyLength);
			ptr += bodyLength;
		}
		// Number of bytes written shall equal vec.size()
		assert(static_cast<std::size_t>(ptr - vec.data()) == vec.size());
		return vec;
	}();
	// Write final binary replay
	bytes.resize(sizeof(ReplayHeader) + pthData1.size());
	uint8_t* ptr = bytes.data();
	Write<ReplayHeader>(ptr, header1);
	std::memcpy(ptr, pthData1.data(), pthData1.size());
// 	ptr += pthData1.size();
	// Size occupied by all player responses
// 	header.size += [&]()
// 	{
// 		std::size_t v{};
// 		for(const auto& r : responses)
// 			v += r.size() + 1U; // length
// 		return static_cast<uint32_t>(v);
// 	}();
	// Size occupied by each duelist and their decks
// 	header.size += [&]()
// 	{
// 		constexpr std::size_t BASE =
// 			40U + // name<2 * 20>
// 			8U;   // deckCount<4> + extraCount<4>
// 		std::size_t v{};
// 		for(const auto& l : duelists)
// 			for(const auto& d : l)
// 				v += BASE + (d.main.size() * 4U) + (d.extra.size() * 4U);
// 		return static_cast<uint32_t>(v);
// 	}();
}

// ***** YRPX Binary format *****
// ReplayHeader
// team0Count [uint32_t]
// team0Names [20 uint16_t * team0Count]
// team1Count [uint32_t]
// team1Names [20 uint16_t * team1Count]
// duelFlags [uint32_t]
// Core messages: uint8_t msgType, followed by uint32_t length of message, followed by uint8_t * length
//

// ***** YRP Binary format *****
//
// ReplayHeader
// team0Count [uint32_t]
// team0Names [20 uint16_t * team0Count]
// team1Count [uint32_t]
// team1Names [20 uint16_t * team1Count]
// startingLP [uint32_t]
// startingDrawCount [uint32_t]
// drawCountPerTurn [uint32_t]
// duelFlags [uint32_t]
// DECKS
// EXTRA_CARDS
// Core responses: uint8_t length, followed by uint8_t * length

} // namespace YGOPro
