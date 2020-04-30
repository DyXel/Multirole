#include "CoreUtils.hpp"

#include <stdexcept> // std::out_of_range

#include "Msgs.inl"

namespace YGOPro::CoreUtils
{

struct LocInfo
{
	static constexpr std::size_t Size = 1 + 1 + 4 + 4;
	uint8_t con;  // Controller
	uint8_t loc;  // Location
	uint32_t seq; // Sequence
	uint32_t pos; // Position
};

template<typename T>
inline T Read(const uint8_t*& ptr)
{
	T value;
	std::memcpy(&value, ptr, sizeof(T));
	ptr += sizeof(T);
	return value;
}

template<typename T>
inline T Read(uint8_t*& ptr)
{
	T value;
	std::memcpy(&value, ptr, sizeof(T));
	ptr += sizeof(T);
	return value;
}

template<>
inline LocInfo Read(uint8_t*& ptr)
{
	return LocInfo
	{
		Read<uint8_t>(ptr),
		Read<uint8_t>(ptr),
		Read<uint32_t>(ptr),
		Read<uint32_t>(ptr)
	};
}

template<typename T>
inline void Write(uint8_t*& ptr, T value)
{
	std::memcpy(ptr, &value, sizeof(T));
	ptr += sizeof(T);
}

std::vector<Msg> SplitToMsgs(const Buffer& buffer)
{
	using length_t = uint32_t;
	static constexpr std::size_t sizeOfLength = sizeof(length_t);
	std::vector<Msg> msgs;
	if(buffer.empty())
		return msgs;
	const std::size_t bufSize = buffer.size();
	const uint8_t* const bufData = buffer.data();
	for(std::size_t pos = 0; pos != bufSize; )
	{
		// Retrieve length of this message
		if(pos + sizeOfLength > bufSize)
			throw std::out_of_range("SplitMessages: Reading length");
		length_t length;
		std::memcpy(&length, bufData + pos, sizeOfLength);
		pos += sizeOfLength;
		// Copy message data to a new message
		if(pos + length > bufSize)
			throw std::out_of_range("SplitMessages: Reading message data");
		auto& msg = msgs.emplace_back(length);
		std::memcpy(msg.data(), bufData + pos, length);
		pos += length;
	}
	return msgs;
}

uint8_t GetMessageType(const Msg& msg)
{
	return msg.at(0);
}

bool DoesMessageRequireAnswer(uint8_t msgType)
{
	switch(msgType)
	{
	case MSG_SELECT_CARD:
	case MSG_SELECT_TRIBUTE:
	case MSG_SELECT_UNSELECT_CARD:
	case MSG_SELECT_BATTLECMD:
	case MSG_SELECT_IDLECMD:
	case MSG_SELECT_EFFECTYN:
	case MSG_SELECT_YESNO:
	case MSG_SELECT_OPTION:
	case MSG_SELECT_CHAIN:
	case MSG_SELECT_PLACE:
	case MSG_SELECT_DISFIELD:
	case MSG_SELECT_POSITION:
	case MSG_SORT_CHAIN:
	case MSG_SELECT_COUNTER:
	case MSG_SELECT_SUM:
	case MSG_ROCK_PAPER_SCISSORS:
	case MSG_ANNOUNCE_RACE:
	case MSG_ANNOUNCE_ATTRIB:
	case MSG_ANNOUNCE_CARD:
	case MSG_ANNOUNCE_NUMBER:
	case MSG_ANNOUNCE_CARD_FILTER:
	{
		return true;
	}
	default:
	{
		return false;
	}
	}
}

MsgDistType GetMessageDistributionType(const Msg& msg)
{
	switch(GetMessageType(msg))
	{
	case MSG_SELECT_CARD:
	case MSG_SELECT_TRIBUTE:
	case MSG_SELECT_UNSELECT_CARD:
	{
		return MsgDistType::MSG_DIST_TYPE_SPECIFIC_TEAM_DUELIST_STRIPPED;
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
	case MSG_SORT_CHAIN:
	case MSG_SELECT_COUNTER:
	case MSG_SELECT_SUM:
	case MSG_ROCK_PAPER_SCISSORS:
	case MSG_ANNOUNCE_RACE:
	case MSG_ANNOUNCE_ATTRIB:
	case MSG_ANNOUNCE_CARD:
	case MSG_ANNOUNCE_NUMBER:
	case MSG_ANNOUNCE_CARD_FILTER:
	case MSG_MISSED_EFFECT:
	{
		return MsgDistType::MSG_DIST_TYPE_SPECIFIC_TEAM_DUELIST;
	}
	case MSG_HINT:
	{
		switch(msg.at(1))
		{
		case 1: case 2: case 3: case 5:
		{
			return MsgDistType::MSG_DIST_TYPE_SPECIFIC_TEAM_DUELIST;
		}
		case 200:
		{
			return MsgDistType::MSG_DIST_TYPE_SPECIFIC_TEAM;
		}
		case 4: case 6: case 7: case 8: case 9: case 11:
		{
			return MsgDistType::MSG_DIST_TYPE_EVERYONE_EXCEPT_TEAM_DUELIST;
		}
		default: /*case 10: case 201: case 202: case 203:*/
		{
			return MsgDistType::MSG_DIST_TYPE_EVERYONE;
		}
		}
	}
	case MSG_CONFIRM_CARDS:
	{
		if(msg.size() < 2 + 4)
			throw std::out_of_range("MSG_CONFIRM_CARDS#1");
		auto ptr = msg.data() + 2;
		// if count(uint32_t) is not 0 and location(uint8_t) is LOCATION_DECK
		// then send to specific team duelist.
		if(Read<uint32_t>(ptr) != 0)
		{
			if(msg.size() < 2 + 4 + 4 + 1 + 1)
				throw std::out_of_range("MSG_CONFIRM_CARDS#2");
			ptr += 4 + 1;
			if(Read<uint8_t>(ptr) == 0x01) // NOLINT: LOCATION_DECK
				return MsgDistType::MSG_DIST_TYPE_SPECIFIC_TEAM_DUELIST;
		}
		return MsgDistType::MSG_DIST_TYPE_EVERYONE;
	}
	case MSG_SHUFFLE_HAND:
	case MSG_SHUFFLE_EXTRA:
	case MSG_SET:
	case MSG_MOVE:
	case MSG_DRAW:
	case MSG_TAG_SWAP:
	{
		return MsgDistType::MSG_DIST_TYPE_EVERYONE_STRIPPED;
	}
	default:
	{
		return MsgDistType::MSG_DIST_TYPE_EVERYONE;
	}
	}
}

uint8_t GetMessageReceivingTeam(const Msg& msg)
{
	switch(GetMessageType(msg))
	{
	case MSG_HINT:
	{
		return msg.at(2);
	}
	default:
	{
		return msg.at(1);
	}
	}
}

Msg StripMessageForTeam(uint8_t team, Msg msg)
{
	auto IsLocInfoPublic = [](const LocInfo& info)
	{
		// NOLINTNEXTLINE: different LOCATION_ constants
		if(info.loc & (0x10 | 0x80) && !(info.loc & (0x01 | 0x02)))
			return true;
		else if(!(info.pos & 0x0A)) // NOLINT: POS_FACEDOWN
			return true;
		return false;
	};
	auto ClearPositionArray = [](uint32_t count, uint8_t*& ptr)
	{
		for(uint32_t i = 0; i < count; i++)
		{
			ptr += 4; // Card code
			auto pos = Read<uint32_t>(ptr);
			if(!(pos & 0x05)) // NOLINT: POS_FACEUP
			{
				ptr -= 8;
				Write<uint32_t>(ptr, 0);
				ptr += 4;
			}
		}
	};
	auto CheckLength = [l = msg.size()](std::size_t expected, const char* what)
	{
		if(l < expected)
			throw std::out_of_range(what);
	};
	auto ptr = msg.data();
	ptr++; // type ignored
	switch(GetMessageType(msg))
	{
	case MSG_SET:
	{
		CheckLength(1 + 4, "MSG_SET is too short");
		Write<uint32_t>(ptr, 0);
		break;
	}
	case MSG_SHUFFLE_HAND:
	case MSG_SHUFFLE_EXTRA:
	{
		CheckLength(1 + 1 + 4, "MSG_SHUFFLE_*#1");
		if(Read<uint8_t>(ptr) == team)
			break;
		auto count = Read<uint32_t>(ptr);
		CheckLength(1 + 1 + 4 + (count * 4), "MSG_SHUFFLE_*#2");
		for(uint32_t i = 0; i < count; i++)
			Write<uint32_t>(ptr, 0);
		break;
	}
	case MSG_MOVE:
	{
		CheckLength(1 + 4 + (LocInfo::Size * 2), "MSG_MOVE#1");
		ptr += 4; // Card code
		ptr += LocInfo::Size; // Previous location
		const auto current = Read<LocInfo>(ptr);
		if(current.con == team || IsLocInfoPublic(current))
			break;
		ptr -= 4 + (LocInfo::Size * 2);
		Write<uint32_t>(ptr, 0);
		break;
	}
	case MSG_DRAW:
	{
		CheckLength(1 + 1 + 4, "MSG_DRAW#1");
		if(Read<uint8_t>(ptr) == team)
			break;
		auto count = Read<uint32_t>(ptr);
		CheckLength(1 + 1 + 4 + (count * (4 + 4)), "MSG_DRAW#2");
		ClearPositionArray(count, ptr);
		break;
	}
	case MSG_TAG_SWAP:
	{
		CheckLength(1 + 1 + (4 * 5), "MSG_TAG_SWAP#1");
		if(Read<uint8_t>(ptr) == team)
			break;
		ptr        += 4;                   // Main deck count
		auto count  = Read<uint32_t>(ptr); // Extra deck count
		ptr        += 4;                   // Face-up pendulum count
		count      += Read<uint32_t>(ptr); // Hand count
		ptr        += 4;                   // Top-deck card code
		CheckLength(1 + 1 + (4 * 5) + (count * (4 + 4)), "MSG_TAG_SWAP#2");
		ClearPositionArray(count, ptr);
		break;
	}
	}
	return msg;
}

Msg MakeStartMsg(const MsgStartCreateInfo& info)
{
	Msg msg(18);
	auto ptr = msg.data();
	Write<uint8_t>(ptr, MSG_START);
	Write<uint8_t>(ptr, 0);
	Write<uint32_t>(ptr, info.lp);
	Write<uint32_t>(ptr, info.lp);
	Write(ptr, static_cast<uint16_t>(info.t0DSz));
	Write(ptr, static_cast<uint16_t>(info.t0EdSz));
	Write(ptr, static_cast<uint16_t>(info.t1DSz));
	Write(ptr, static_cast<uint16_t>(info.t1EdSz));
	return msg;
}

STOCMsg GameMsgFromMsg(const Msg& msg)
{
	return STOCMsg{STOCMsg::MsgType::GAME_MSG, msg};
}

} // namespace YGOPro::CoreUtils
