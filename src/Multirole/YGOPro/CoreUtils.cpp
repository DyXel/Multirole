#include "CoreUtils.hpp"

#include <cstring> // std::memcpy
#include <stdexcept> // std::out_of_range

#include "Constants.hpp"

namespace YGOPro::CoreUtils
{

#include "../../Read.inl"
#include "../../Write.inl"

template<>
constexpr LocInfo Read(const uint8_t*& ptr) noexcept
{
	return LocInfo
	{
		Read<uint8_t>(ptr),
		Read<uint8_t>(ptr),
		Read<uint32_t>(ptr),
		Read<uint32_t>(ptr)
	};
}

/*** Query utility functions ***/

inline void AddRefreshAllDecks(std::vector<QueryRequest>& qreqs) noexcept
{
	qreqs.emplace_back(QueryLocationRequest{0U, LOCATION_DECK, 0x1181FFF});
	qreqs.emplace_back(QueryLocationRequest{1U, LOCATION_DECK, 0x1181FFF});
}

inline void AddRefreshAllHands(std::vector<QueryRequest>& qreqs) noexcept
{
	qreqs.emplace_back(QueryLocationRequest{0U, LOCATION_HAND, 0x3781FFF});
	qreqs.emplace_back(QueryLocationRequest{1U, LOCATION_HAND, 0x3781FFF});
}

inline void AddRefreshAllMZones(std::vector<QueryRequest>& qreqs) noexcept
{
	qreqs.emplace_back(QueryLocationRequest{0U, LOCATION_MZONE, 0x3881FFF});
	qreqs.emplace_back(QueryLocationRequest{1U, LOCATION_MZONE, 0x3881FFF});
}

inline void AddRefreshAllSZones(std::vector<QueryRequest>& qreqs) noexcept
{
	qreqs.emplace_back(QueryLocationRequest{0U, LOCATION_SZONE, 0x3E81FFF});
	qreqs.emplace_back(QueryLocationRequest{1U, LOCATION_SZONE, 0x3E81FFF});
}

inline QueryOpt DeserializeOneQuery(const uint8_t*& ptr) noexcept
{
	if(Read<uint16_t>(ptr) == 0U)
		return std::nullopt;
	ptr -= sizeof(uint16_t);
	for(QueryOpt q = Query{};;)
	{
		auto size = Read<uint16_t>(ptr);
		auto flag = Read<uint32_t>(ptr);
		q->flags |= flag;
		switch(flag)
		{
#define X(qtype, var) case qtype: { var = Read<decltype(var)>(ptr); break; }
			X(QUERY_CODE, q->code)
			X(QUERY_POSITION, q->pos)
			X(QUERY_ALIAS, q->alias)
			X(QUERY_TYPE, q->type)
			X(QUERY_LEVEL, q->level)
			X(QUERY_RANK, q->rank)
			X(QUERY_ATTRIBUTE, q->attribute)
			X(QUERY_RACE, q->race)
			X(QUERY_ATTACK, q->attack)
			X(QUERY_DEFENSE, q->defense)
			X(QUERY_BASE_ATTACK, q->bAttack)
			X(QUERY_BASE_DEFENSE, q->bDefense)
			X(QUERY_REASON, q->reason)
			X(QUERY_OWNER, q->owner)
			X(QUERY_STATUS, q->status)
			X(QUERY_IS_PUBLIC, q->isPublic)
			X(QUERY_LSCALE, q->lscale)
			X(QUERY_RSCALE, q->rscale)
			X(QUERY_REASON_CARD, q->reasonCard)
			X(QUERY_EQUIP_CARD, q->equipCard)
			X(QUERY_IS_HIDDEN, q->isHidden)
			X(QUERY_COVER, q->cover)
#undef X
#define X(qtype, var) \
	case qtype: \
	{ \
		auto c = Read<uint32_t>(ptr); \
		for(uint32_t i = 0U; i < c; i++) \
			var.push_back(Read<decltype(var)::value_type>(ptr)); \
		break; \
	}
			X(QUERY_TARGET_CARD, q->targets)
			X(QUERY_OVERLAY_CARD, q->overlays)
			X(QUERY_COUNTERS, q->counters)
#undef X
			case QUERY_LINK:
			{
				q->link = Read<uint32_t>(ptr);
				q->linkMarker = Read<uint32_t>(ptr);
				break;
			}
			case QUERY_END:
			{
				return q;
			}
			default:
			{
				ptr += size - sizeof(uint32_t);
				break;
			}
		}
	}
}

/*** Header implementations ***/

std::vector<Msg> SplitToMsgs(const Buffer& buffer) noexcept
{
	using length_t = uint32_t;
	static constexpr std::size_t sizeOfLength = sizeof(length_t);
	std::vector<Msg> msgs;
	if(buffer.empty())
		return msgs;
	const std::size_t bufSize = buffer.size();
	const uint8_t* const bufData = buffer.data();
	for(std::size_t pos = 0U; pos != bufSize; )
	{
		// Retrieve length of this message
		length_t l = 0U;
		std::memcpy(&l, bufData + pos, sizeOfLength);
		pos += sizeOfLength;
		// Copy message data to a new message
		auto& msg = msgs.emplace_back(l);
		std::memcpy(msg.data(), bufData + pos, l);
		pos += l;
	}
	return msgs;
}

uint8_t GetMessageType(const Msg& msg) noexcept
{
	return msg[0U];
}

bool DoesMessageRequireAnswer(uint8_t msgType) noexcept
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
	case MSG_SORT_CARD:
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

MsgDistType GetMessageDistributionType(const Msg& msg) noexcept
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
	case MSG_SORT_CARD:
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
		switch(msg[1U])
		{
		case 1U: case 2U: case 3U: case 5U:
		{
			return MsgDistType::MSG_DIST_TYPE_SPECIFIC_TEAM_DUELIST;
		}
		case 200U:
		{
			return MsgDistType::MSG_DIST_TYPE_SPECIFIC_TEAM;
		}
		case 4U: case 6U: case 7U: case 8U: case 9U: case 11U:
		{
			return MsgDistType::MSG_DIST_TYPE_EVERYONE_EXCEPT_TEAM_DUELIST;
		}
		default: /*case 10U: case 201U: case 202U: case 203U:*/
		{
			return MsgDistType::MSG_DIST_TYPE_EVERYONE;
		}
		}
	}
	case MSG_CONFIRM_CARDS:
	{
		const auto* ptr = msg.data() + 2U;
		// if count(uint32_t) is not 0 and location(uint8_t) is LOCATION_DECK
		// then send to specific team duelist.
		if(Read<uint32_t>(ptr) != 0U)
		{
			ptr += 4U + 1U;
			if(Read<uint8_t>(ptr) == LOCATION_DECK)
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

uint8_t GetMessageReceivingTeam(const Msg& msg) noexcept
{
	switch(GetMessageType(msg))
	{
	case MSG_HINT:
	{
		return msg[2U];
	}
	default:
	{
		return msg[1U];
	}
	}
}

Msg StripMessageForTeam(uint8_t team, Msg msg) noexcept
{
	auto IsLocInfoPublic = [](const LocInfo& info)
	{
		if(info.loc & (LOCATION_GRAVE | LOCATION_OVERLAY) &&
		   !(info.loc & (LOCATION_DECK | LOCATION_HAND)))
			return true;
		if(!(info.pos & POS_FACEDOWN))
			return true;
		return false;
	};
	auto ClearPositionArray = [](uint32_t count, uint8_t*& ptr)
	{
		for(uint32_t i = 0U; i < count; i++)
		{
			ptr += 4U; // Card code
			auto pos = Read<uint32_t>(ptr);
			if(!(pos & POS_FACEUP))
			{
				ptr -= 4U + 4U;
				Write<uint32_t>(ptr, 0U);
				ptr += 4U;
			}
		}
	};
	auto ClearLocInfoArray = [](uint32_t count, uint8_t team, uint8_t*& ptr)
	{
		for(uint32_t i = 0U; i < count; i++)
		{
			ptr += 4U; // Card code
			const auto info = Read<LocInfo>(ptr);
			if(team != info.con)
			{
				ptr -= LocInfo::SIZE + 4U;
				Write<uint32_t>(ptr, 0U);
				ptr += LocInfo::SIZE;
			}
		}
	};
	auto* ptr = msg.data();
	ptr++; // type ignored
	switch(GetMessageType(msg))
	{
	case MSG_SET:
	{
		Write<uint32_t>(ptr, 0U);
		break;
	}
	case MSG_SHUFFLE_HAND:
	case MSG_SHUFFLE_EXTRA:
	{
		if(Read<uint8_t>(ptr) == team)
			break;
		auto count = Read<uint32_t>(ptr);
		for(uint32_t i = 0U; i < count; i++)
			Write<uint32_t>(ptr, 0U);
		break;
	}
	case MSG_MOVE:
	{
		ptr += 4U; // Card code
		ptr += LocInfo::SIZE; // Previous location
		const auto current = Read<LocInfo>(ptr);
		if(current.con == team || IsLocInfoPublic(current))
			break;
		ptr -= 4U + (LocInfo::SIZE * 2U);
		Write<uint32_t>(ptr, 0U);
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
		ptr        += 4U;                   // Main deck count
		auto count  = Read<uint32_t>(ptr);  // Extra deck count
		ptr        += 4U;                   // Face-up pendulum count
		count      += Read<uint32_t>(ptr);  // Hand count
		ptr        += 4U;                   // Top-deck card code
		ClearPositionArray(count, ptr);
		break;
	}
	case MSG_SELECT_CARD:
	{
		ptr += 1U + 1U + 4U + 4U;
		auto count = Read<uint32_t>(ptr);
		ClearLocInfoArray(count, team, ptr);
		break;
	}
	case MSG_SELECT_TRIBUTE:
	{
		ptr += 1U + 1U + 4U + 4U;
		auto count = Read<uint32_t>(ptr);
		for(uint32_t i = 0; i < count; i++)
		{
			ptr += 4U; // Card code
			const auto con = Read<uint8_t>(ptr);
			if(team != con)
			{
				ptr -= 4U + 1U;
				Write<uint32_t>(ptr, 0U);
				ptr += 1U;
			}
			ptr += 1U + 4U + 1U; // loc, seq, release_param
		}
		break;
	}
	case MSG_SELECT_UNSELECT_CARD:
	{
		ptr += 1U + 1U + 1U + 4U + 4U;
		auto count1 = Read<uint32_t>(ptr);
		ClearLocInfoArray(count1, team, ptr);
		auto count2 = Read<uint32_t>(ptr);
		ClearLocInfoArray(count2, team, ptr);
		break;
	}
	}
	return msg;
}

Msg MakeStartMsg(const MsgStartCreateInfo& info) noexcept
{
	Msg msg(18U);
	auto* ptr = msg.data();
	Write<uint8_t>(ptr, MSG_START);
	Write<uint8_t>(ptr, 0U);
	Write<uint32_t>(ptr, info.lp);
	Write<uint32_t>(ptr, info.lp);
	Write(ptr, static_cast<uint16_t>(info.t0DSz));
	Write(ptr, static_cast<uint16_t>(info.t0EdSz));
	Write(ptr, static_cast<uint16_t>(info.t1DSz));
	Write(ptr, static_cast<uint16_t>(info.t1EdSz));
	return msg;
}

std::vector<QueryRequest> GetPreDistQueryRequests(const Msg& msg) noexcept
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
		const auto* ptr = msg.data();
		ptr++; // type ignored
		ptr += 4U; // Card code
		const auto i = Read<LocInfo>(ptr);
		qreqs.emplace_back(QuerySingleRequest{i.con, i.loc, i.seq, 0x3F81FFF});
		break;
	}
	}
	return qreqs;
}

std::vector<QueryRequest> GetPostDistQueryRequests(const Msg& msg) noexcept
{
	const auto* ptr = msg.data();
	ptr++; // type ignored
	std::vector<QueryRequest> qreqs;
	switch(GetMessageType(msg))
	{
	case MSG_SHUFFLE_HAND:
	case MSG_DRAW:
	{
		auto player = Read<uint8_t>(ptr);
		qreqs.emplace_back(QueryLocationRequest{player, LOCATION_HAND, 0x3781FFF});
		break;
	}
	case MSG_SHUFFLE_EXTRA:
	{
		auto player = Read<uint8_t>(ptr);
		qreqs.emplace_back(QueryLocationRequest{player, LOCATION_EXTRA, 0x381FFF});
		break;
	}
	case MSG_SWAP_GRAVE_DECK:
	{
		auto player = Read<uint8_t>(ptr);
		qreqs.emplace_back(QueryLocationRequest{player, LOCATION_GRAVE, 0x381FFF});
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
		qreqs.emplace_back(QueryLocationRequest{0U, loc, 0x3181FFF});
		qreqs.emplace_back(QueryLocationRequest{1U, loc, 0x3181FFF});
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
		ptr += 4U; // Card code
		const auto previous = Read<LocInfo>(ptr);
		const auto current = Read<LocInfo>(ptr);
		if((previous.con != current.con || previous.loc != current.loc) &&
		   current.loc != 0U && (current.loc & LOCATION_OVERLAY) == 0U)
		{
			qreqs.emplace_back(QuerySingleRequest{
				current.con,
				current.loc,
				current.seq,
				0x3F81FFF});
		}
		break;
	}
	case MSG_POS_CHANGE:
	{
		ptr += 4U; // Card code
		auto cc = Read<uint8_t>(ptr); // Current controller
		auto cl = Read<uint8_t>(ptr); // Current location
		auto cs = Read<uint8_t>(ptr); // Current sequence
		auto pp = Read<uint8_t>(ptr); // Previous position
		auto cp = Read<uint8_t>(ptr); // Current position
		if((pp & POS_FACEDOWN) && (cp & POS_FACEUP))
			qreqs.emplace_back(QuerySingleRequest{cc, cl, cs, 0x3F81FFF});
		break;
	}
	case MSG_SWAP:
	{
		ptr += 4U; // Previous card code
		const auto p = Read<LocInfo>(ptr);
		ptr += 4U; // Current card code
		const auto c = Read<LocInfo>(ptr);
		qreqs.emplace_back(QuerySingleRequest{p.con, p.loc, p.seq, 0x3F81FFF});
		qreqs.emplace_back(QuerySingleRequest{c.con, c.loc, c.seq, 0x3F81FFF});
		break;
	}
	case MSG_TAG_SWAP:
	{
		auto player = Read<uint8_t>(ptr);
		qreqs.reserve(7U);
		qreqs.emplace_back(QueryLocationRequest{player, LOCATION_DECK, 0x1181FFF});
		qreqs.emplace_back(QueryLocationRequest{player, LOCATION_EXTRA, 0x381FFF});
		qreqs.emplace_back(QueryLocationRequest{player, LOCATION_HAND, 0x3781FFF});
		qreqs.emplace_back(QueryLocationRequest{0U, LOCATION_MZONE, 0x3081FFF});
		qreqs.emplace_back(QueryLocationRequest{1U, LOCATION_MZONE, 0x3081FFF});
		qreqs.emplace_back(QueryLocationRequest{0U, LOCATION_SZONE, 0x30681FFF});
		qreqs.emplace_back(QueryLocationRequest{1U, LOCATION_SZONE, 0x30681FFF});
		break;
	}
	case MSG_RELOAD_FIELD:
	{
		qreqs.emplace_back(QueryLocationRequest{0U, LOCATION_EXTRA, 0x381FFF});
		qreqs.emplace_back(QueryLocationRequest{1U, LOCATION_EXTRA, 0x381FFF});
		break;
	}
	}
	return qreqs;
}

Msg MakeUpdateCardMsg(uint8_t con, uint32_t loc, uint32_t seq, const QueryBuffer& qb) noexcept
{
	Msg msg(1U + 1U + 1U + 1U + qb.size());
	auto* ptr = msg.data();
	Write<uint8_t>(ptr, MSG_UPDATE_CARD);
	Write<uint8_t>(ptr, con);
	Write(ptr, static_cast<uint8_t>(loc));
	Write(ptr, static_cast<uint8_t>(seq));
	std::memcpy(ptr, qb.data(), qb.size());
	return msg;
}

Msg MakeUpdateDataMsg(uint8_t con, uint32_t loc, const QueryBuffer& qb) noexcept
{
	Msg msg(1U + 1U + 1U + qb.size());
	auto* ptr = msg.data();
	Write<uint8_t>(ptr, MSG_UPDATE_DATA);
	Write<uint8_t>(ptr, con);
	Write(ptr, static_cast<uint8_t>(loc));
	std::memcpy(ptr, qb.data(), qb.size());
	return msg;
}

QueryOpt DeserializeSingleQueryBuffer(const QueryBuffer& qb) noexcept
{
	const auto* ptr = qb.data();
	return DeserializeOneQuery(ptr);
}

QueryOptVector DeserializeLocationQueryBuffer(const QueryBuffer& qb) noexcept
{
	const auto* ptr = qb.data();
	const auto* const ptrMax = ptr + Read<uint32_t>(ptr);
	QueryOptVector ret;
	while(ptr < ptrMax)
		ret.emplace_back(DeserializeOneQuery(ptr));
	return ret;
}

QueryBuffer SerializeSingleQuery(const QueryOpt& qOpt, bool isPublic) noexcept
{
	QueryBuffer qb;
	if(!qOpt.has_value()) // Nothing to serialize.
	{
		qb.resize(sizeof(uint16_t), uint8_t{0U});
		return qb;
	}
	const auto& q = qOpt.value();
	// Check if a certain query is public or if the whole query object
	// itself is public.
	auto IsPublic = [&q](uint64_t flag) constexpr -> bool
	{
		if((q.flags & QUERY_IS_PUBLIC) && q.isPublic)
			return true;
		if((q.flags & QUERY_POSITION) && (q.pos & POS_FACEUP))
			return true;
		switch(flag)
		{
			case QUERY_CODE:
			case QUERY_ALIAS:
			case QUERY_TYPE:
			case QUERY_LEVEL:
			case QUERY_RANK:
			case QUERY_ATTRIBUTE:
			case QUERY_RACE:
			case QUERY_ATTACK:
			case QUERY_DEFENSE:
			case QUERY_BASE_ATTACK:
			case QUERY_BASE_DEFENSE:
			case QUERY_STATUS:
			case QUERY_LSCALE:
			case QUERY_RSCALE:
			case QUERY_LINK:
			{
				return false;
			}
			default:
			{
				return true;
			}
		}
	};
	auto ComputeQuerySize = [&q](uint64_t flag) constexpr -> std::size_t
	{
		switch(flag)
		{
			case QUERY_OWNER:
			case QUERY_IS_PUBLIC:
			case QUERY_IS_HIDDEN:
			{
				return sizeof(uint8_t);
			}
			case QUERY_CODE:
			case QUERY_POSITION:
			case QUERY_ALIAS:
			case QUERY_TYPE:
			case QUERY_LEVEL:
			case QUERY_RANK:
			case QUERY_ATTRIBUTE:
			case QUERY_ATTACK:
			case QUERY_DEFENSE:
			case QUERY_BASE_ATTACK:
			case QUERY_BASE_DEFENSE:
			case QUERY_REASON:
			case QUERY_STATUS:
			case QUERY_LSCALE:
			case QUERY_RSCALE:
			case QUERY_COVER:
			{
				return sizeof(uint32_t);
			}
			case QUERY_RACE:
			{
				return sizeof(uint64_t);
			}
			case QUERY_REASON_CARD:
			case QUERY_EQUIP_CARD:
			{
				return LocInfo::SIZE;
			}
#define X(qtype, var) \
	case qtype: \
	{ \
		return sizeof(uint32_t) + \
		       (var.size() * sizeof(decltype(var)::value_type)); \
		break; \
	}
			X(QUERY_TARGET_CARD, q.targets)
			X(QUERY_OVERLAY_CARD, q.overlays)
			X(QUERY_COUNTERS, q.counters)
#undef X
			case QUERY_LINK:
			{
				return sizeof(uint32_t) + sizeof(uint32_t);
			}
			case QUERY_END:
			default:
			{
				return 0U;
			}
		}
	};
	const auto totalQuerySize = [&]() constexpr -> std::size_t
	{
		std::size_t size = 0U;
		for(uint64_t flag = 1U; flag <= QUERY_END; flag <<= 1U)
		{
			if((q.flags & flag) != flag)
				continue;
			if(flag == QUERY_REASON_CARD && q.reasonCard.loc == 0U)
				continue;
			if(flag == QUERY_EQUIP_CARD && q.equipCard.loc == 0U)
				continue;
			if((q.flags & QUERY_IS_HIDDEN) && q.isHidden && !IsPublic(flag))
				continue;
			if(isPublic && !IsPublic(flag))
				continue;
			size += sizeof(uint16_t) + sizeof(uint32_t);
			size += ComputeQuerySize(flag);
		}
		return size;
	}();
	qb.resize(totalQuerySize);
	auto Insert = [ptr = qb.data()](auto&& value) mutable
	{
		using Base = std::remove_cv_t<std::remove_reference_t<decltype(value)>>;
		if constexpr(std::is_same_v<Base, LocInfo>)
		{
			Write<uint8_t>(ptr, value.con);
			Write<uint8_t>(ptr, value.loc);
			Write<uint32_t>(ptr, value.seq);
			Write<uint32_t>(ptr, value.pos);
		}
		else
		{
			Write<Base>(ptr, value);
		}
	};
	for(uint64_t flag = 1U; flag <= QUERY_END; flag <<= 1U)
	{
		if((q.flags & flag) != flag)
			continue;
		if(flag == QUERY_REASON_CARD && q.reasonCard.loc == 0U)
			continue;
		if(flag == QUERY_EQUIP_CARD && q.equipCard.loc == 0U)
			continue;
		if((q.flags & QUERY_IS_HIDDEN) && q.isHidden && !IsPublic(flag))
			continue;
		if(isPublic && !IsPublic(flag))
			continue;
		Insert(static_cast<uint16_t>(ComputeQuerySize(flag) + sizeof(uint32_t)));
		Insert(static_cast<uint32_t>(flag));
		switch(flag)
		{
#define X(qtype, var) case qtype: { Insert(var); break; }
			X(QUERY_CODE, q.code)
			X(QUERY_POSITION, q.pos)
			X(QUERY_ALIAS, q.alias)
			X(QUERY_TYPE, q.type)
			X(QUERY_LEVEL, q.level)
			X(QUERY_RANK, q.rank)
			X(QUERY_ATTRIBUTE, q.attribute)
			X(QUERY_RACE, q.race)
			X(QUERY_ATTACK, q.attack)
			X(QUERY_DEFENSE, q.defense)
			X(QUERY_BASE_ATTACK, q.bAttack)
			X(QUERY_BASE_DEFENSE, q.bDefense)
			X(QUERY_REASON, q.reason)
			X(QUERY_OWNER, q.owner)
			X(QUERY_STATUS, q.status)
			X(QUERY_IS_PUBLIC, q.isPublic)
			X(QUERY_LSCALE, q.lscale)
			X(QUERY_RSCALE, q.rscale)
			X(QUERY_REASON_CARD, q.reasonCard)
			X(QUERY_EQUIP_CARD, q.equipCard)
			X(QUERY_IS_HIDDEN, q.isHidden)
			X(QUERY_COVER, q.cover)
#undef X
#define X(qtype, var) \
	case qtype: \
	{ \
		Insert(static_cast<uint16_t>(var.size())); \
		for(const auto& elem : var) \
			Insert(elem); \
		break; \
	}
			X(QUERY_TARGET_CARD, q.targets)
			X(QUERY_OVERLAY_CARD, q.overlays)
			X(QUERY_COUNTERS, q.counters)
#undef X
			case QUERY_LINK:
			{
				Insert(q.link);
				Insert(q.linkMarker);
				break;
			}
			default:
				break;
		}
	}
	return qb;
}

QueryBuffer SerializeLocationQuery(const QueryOptVector& qs, bool isPublic) noexcept
{
	uint32_t totalSize = 0U;
	QueryBuffer qb(sizeof(decltype(totalSize)));
	for(const auto& q : qs)
	{
		const auto singleQb = SerializeSingleQuery(q, isPublic);
		totalSize += static_cast<uint32_t>(singleQb.size());
		qb.insert(qb.end(), singleQb.begin(), singleQb.end());
	}
	std::memcpy(qb.data(), &totalSize, sizeof(decltype(totalSize)));
	return qb;
}

} // namespace YGOPro::CoreUtils
