#include "CoreUtils.hpp"

#include <stdexcept> // std::out_of_range

#include "Msgs.inl"

namespace YGOPro::CoreUtils
{

struct LocInfo
{
	static constexpr std::size_t SIZE = 1 + 1 + 4 + 4;
	uint8_t con;  // Controller
	uint8_t loc;  // Location
	uint32_t seq; // Sequence
	uint32_t pos; // Position
};

template<typename T>
inline T Read(const uint8_t*& ptr)
{
	T value{};
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

/*** Query utility functions ***/

inline void AddRefreshAllDecks(std::vector<QueryRequest>& qreqs)
{
	qreqs.emplace_back(QueryLocationRequest{0, 0x01, 0x1181fff});
	qreqs.emplace_back(QueryLocationRequest{1, 0x01, 0x1181fff});
}

inline void AddRefreshAllHands(std::vector<QueryRequest>& qreqs)
{
	qreqs.emplace_back(QueryLocationRequest{0, 0x02, 0x3781fff});
	qreqs.emplace_back(QueryLocationRequest{1, 0x02, 0x3781fff});
}

inline void AddRefreshAllMZones(std::vector<QueryRequest>& qreqs)
{
	qreqs.emplace_back(QueryLocationRequest{0, 0x04, 0x3881fff});
	qreqs.emplace_back(QueryLocationRequest{1, 0x04, 0x3881fff});
}

inline void AddRefreshAllSZones(std::vector<QueryRequest>& qreqs)
{
	qreqs.emplace_back(QueryLocationRequest{0, 0x08, 0x3e81fff});
	qreqs.emplace_back(QueryLocationRequest{1, 0x08, 0x3e81fff});
}

/*** Header implementations ***/

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
		length_t l = 0;
		std::memcpy(&l, bufData + pos, sizeOfLength);
		pos += sizeOfLength;
		// Copy message data to a new message
		auto& msg = msgs.emplace_back(l);
		std::memcpy(msg.data(), bufData + pos, l);
		pos += l;
	}
	return msgs;
}

uint8_t GetMessageType(const Msg& msg)
{
	return msg[0];
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
		switch(msg[1])
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
		const auto *ptr = msg.data() + 2;
		// if count(uint32_t) is not 0 and location(uint8_t) is LOCATION_DECK
		// then send to specific team duelist.
		if(Read<uint32_t>(ptr) != 0)
		{
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
		return msg[2];
	}
	default:
	{
		return msg[1];
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
				ptr -= 4 + 4;
				Write<uint32_t>(ptr, 0);
				ptr += 4;
			}
		}
	};
	auto ClearLocInfoArray = [](uint32_t count, uint8_t team, uint8_t*& ptr)
	{
		for(uint32_t i = 0; i < count; i++)
		{
			ptr += 4; // Card code
			const auto info = Read<LocInfo>(ptr);
			if(team != info.con)
			{
				ptr -= LocInfo::SIZE + 4;
				Write<uint32_t>(ptr, 0);
				ptr += LocInfo::SIZE;
			}
		}
	};
	auto *ptr = msg.data();
	ptr++; // type ignored
	switch(GetMessageType(msg))
	{
	case MSG_SET:
	{
		Write<uint32_t>(ptr, 0);
		break;
	}
	case MSG_SHUFFLE_HAND:
	case MSG_SHUFFLE_EXTRA:
	{
		if(Read<uint8_t>(ptr) == team)
			break;
		auto count = Read<uint32_t>(ptr);
		for(uint32_t i = 0; i < count; i++)
			Write<uint32_t>(ptr, 0);
		break;
	}
	case MSG_MOVE:
	{
		ptr += 4; // Card code
		ptr += LocInfo::SIZE; // Previous location
		const auto current = Read<LocInfo>(ptr);
		if(current.con == team || IsLocInfoPublic(current))
			break;
		ptr -= 4 + (LocInfo::SIZE * 2);
		Write<uint32_t>(ptr, 0);
		break;
	}
	case MSG_DRAW:
	{
		if(Read<uint8_t>(ptr) == team)
			break;
		auto count = Read<uint32_t>(ptr);
		ClearPositionArray(count, ptr);
		break;
	}
	case MSG_TAG_SWAP:
	{
		if(Read<uint8_t>(ptr) == team)
			break;
		ptr        += 4;                   // Main deck count
		auto count  = Read<uint32_t>(ptr); // Extra deck count
		ptr        += 4;                   // Face-up pendulum count
		count      += Read<uint32_t>(ptr); // Hand count
		ptr        += 4;                   // Top-deck card code
		ClearPositionArray(count, ptr);
		break;
	}
	case MSG_SELECT_CARD:
	{
		ptr += 1 + 1 + 4 + 4;
		auto count = Read<uint32_t>(ptr);
		ClearLocInfoArray(count, team, ptr);
		break;
	}
	case MSG_SELECT_TRIBUTE:
	{
		ptr += 1 + 1 + 4 + 4;
		auto count = Read<uint32_t>(ptr);
		for(uint32_t i = 0; i < count; i++)
		{
			ptr += 4; // Card code
			const auto info = Read<LocInfo>(ptr);
			if(team != info.con)
			{
				ptr -= LocInfo::SIZE + 4;
				Write<uint32_t>(ptr, 0);
				ptr += LocInfo::SIZE;
			}
			ptr += 1; // Release param
		}
		break;
	}
	case MSG_SELECT_UNSELECT_CARD:
	{
		ptr += 1 + 1 + 1 + 4 + 4;
		auto count1 = Read<uint32_t>(ptr);
		ClearLocInfoArray(count1, team, ptr);
		auto count2 = Read<uint32_t>(ptr);
		ClearLocInfoArray(count2, team, ptr);
		break;
	}
	}
	return msg;
}

Msg MakeStartMsg(const MsgStartCreateInfo& info)
{
	Msg msg(18);
	auto *ptr = msg.data();
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

std::vector<QueryRequest> GetPreDistQueryRequests(const Msg& msg)
{
	std::vector<QueryRequest> qreqs;
	switch(GetMessageType(msg))
	{
	case MSG_SELECT_BATTLECMD:
	case MSG_SELECT_IDLECMD:
	{
		AddRefreshAllHands(qreqs);
		AddRefreshAllMZones(qreqs);
		AddRefreshAllSZones(qreqs);
		break;
	}
	case MSG_SELECT_CHAIN:
	case MSG_NEW_TURN:
	{
		AddRefreshAllMZones(qreqs);
		AddRefreshAllSZones(qreqs);
		break;
	}
	case MSG_FLIPSUMMONING:
	{
		const auto *ptr = msg.data();
		ptr++; // type ignored
		ptr += 4; // Card code
		const auto i = Read<LocInfo>(ptr);
		qreqs.emplace_back(QuerySingleRequest{i.con, i.loc, i.seq, 0x3f81fff});
		break;
	}
	}
	return qreqs;
}

std::vector<QueryRequest> GetPostDistQueryRequests(const Msg& msg)
{
	const auto *ptr = msg.data();
	ptr++; // type ignored
	std::vector<QueryRequest> qreqs;
	switch(GetMessageType(msg))
	{
	case MSG_SHUFFLE_HAND:
	case MSG_DRAW:
	{
		auto player = Read<uint8_t>(ptr);
		qreqs.emplace_back(QueryLocationRequest{player, 0x02, 0x3781fff});
		break;
	}
	case MSG_SHUFFLE_EXTRA:
	{
		auto player = Read<uint8_t>(ptr);
		qreqs.emplace_back(QueryLocationRequest{player, 0x40, 0x381fff});
		break;
	}
	case MSG_SWAP_GRAVE_DECK:
	{
		auto player = Read<uint8_t>(ptr);
		qreqs.emplace_back(QueryLocationRequest{player, 0x10, 0x381fff});
		break;
	}
	case MSG_REVERSE_DECK:
	{
		AddRefreshAllDecks(qreqs);
		break;
	}
	case MSG_SHUFFLE_SET_CARD:
	{
		auto loc = Read<uint8_t>(ptr);
		qreqs.emplace_back(QueryLocationRequest{0, loc, 0x3181fff});
		qreqs.emplace_back(QueryLocationRequest{1, loc, 0x3181fff});
		break;
	}
	case MSG_DAMAGE_STEP_START:
	case MSG_DAMAGE_STEP_END:
	{
		AddRefreshAllMZones(qreqs);
		break;
	}
	case MSG_SUMMONED:
	case MSG_SPSUMMONED:
	case MSG_FLIPSUMMONED:
	{
		AddRefreshAllMZones(qreqs);
		AddRefreshAllSZones(qreqs);
		break;
	}
	case MSG_NEW_PHASE:
	case MSG_CHAINED:
	{
		AddRefreshAllMZones(qreqs);
		AddRefreshAllSZones(qreqs);
		AddRefreshAllHands(qreqs);
		break;
	}
	case MSG_CHAIN_END:
	{
		AddRefreshAllDecks(qreqs);
		AddRefreshAllMZones(qreqs);
		AddRefreshAllSZones(qreqs);
		AddRefreshAllHands(qreqs);
		break;
	}
	case MSG_MOVE:
	{
		ptr += 4; // Card code
		const auto previous = Read<LocInfo>(ptr);
		const auto current = Read<LocInfo>(ptr);
		if((previous.con != current.con || previous.loc != current.loc) &&
		   current.loc != 0 && (current.loc & 0x80) == 0)
		{
			qreqs.emplace_back(QuerySingleRequest{
				current.con,
				current.loc,
				current.seq,
				0x3f81fff});
		}
		break;
	}
	case MSG_POS_CHANGE:
	{
		ptr += 4; // Card code
		auto cc = Read<uint8_t>(ptr); // Current controller
		auto cl = Read<uint8_t>(ptr); // Current location
		auto cs = Read<uint8_t>(ptr); // Current sequence
		auto pp = Read<uint8_t>(ptr); // Previous position
		auto cp = Read<uint8_t>(ptr); // Current position
		// NOLINTNEXTLINE: POS_FACEDOWN and POS_FACEUP respectively
		if((pp & 0x0A) && (cp & 0x05))
			qreqs.emplace_back(QuerySingleRequest{cc, cl, cs, 0x3f81fff});
		break;
	}
	case MSG_SWAP:
	{
		ptr += 4; // Previous card code
		const auto p = Read<LocInfo>(ptr);
		ptr += 4; // Current card code
		const auto c = Read<LocInfo>(ptr);
		qreqs.emplace_back(QuerySingleRequest{p.con, p.loc, p.seq, 0x3f81fff});
		qreqs.emplace_back(QuerySingleRequest{c.con, c.loc, c.seq, 0x3f81fff});
		break;
	}
	case MSG_TAG_SWAP:
	{
		auto player = Read<uint8_t>(ptr);
		qreqs.reserve(8);
		qreqs.emplace_back(QueryLocationRequest{player, 0x01, 0x1181fff});
		qreqs.emplace_back(QueryLocationRequest{player, 0x40, 0x381fff});
		AddRefreshAllHands(qreqs);
		qreqs.emplace_back(QueryLocationRequest{0, 0x04, 0x3081fff});
		qreqs.emplace_back(QueryLocationRequest{1, 0x04, 0x3081fff});
		qreqs.emplace_back(QueryLocationRequest{0, 0x08, 0x30681fff});
		qreqs.emplace_back(QueryLocationRequest{1, 0x08, 0x30681fff});
		break;
	}
	}
	return qreqs;
}

Msg MakeUpdateCardMsg(uint8_t con, uint32_t loc, uint32_t seq, const Query& q)
{
	Msg msg(1 + 1 + 1 + 1 + q.size());
	auto *ptr = msg.data();
	Write<uint8_t>(ptr, MSG_UPDATE_CARD);
	Write<uint8_t>(ptr, con);
	Write(ptr, static_cast<uint8_t>(loc));
	Write(ptr, static_cast<uint8_t>(seq));
	std::memcpy(ptr, q.data(), q.size());
	return msg;
}

Msg MakeUpdateDataMsg(uint8_t con, uint32_t loc, const Query& q)
{
	Msg msg(1 + 1 + 1 + q.size());
	auto *ptr = msg.data();
	Write<uint8_t>(ptr, MSG_UPDATE_DATA);
	Write<uint8_t>(ptr, con);
	Write(ptr, static_cast<uint8_t>(loc));
	std::memcpy(ptr, q.data(), q.size());
	return msg;
}

} // namespace YGOPro::CoreUtils
