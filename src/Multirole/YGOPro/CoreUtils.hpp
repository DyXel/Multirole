#ifndef YGOPRO_COREUTILS_HPP
#define YGOPRO_COREUTILS_HPP
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace YGOPro::CoreUtils
{

struct LocInfo
{
	static constexpr std::size_t SIZE = 1U + 1U + 4U + 4U;
	uint8_t con;  // Controller
	uint8_t loc;  // Location
	uint32_t seq; // Sequence
	uint32_t pos; // Position
};

enum class MsgDistType
{
	MSG_DIST_TYPE_SPECIFIC_TEAM_DUELIST_STRIPPED,
	MSG_DIST_TYPE_SPECIFIC_TEAM_DUELIST,
	MSG_DIST_TYPE_SPECIFIC_TEAM,
	MSG_DIST_TYPE_EVERYONE_EXCEPT_TEAM_DUELIST,
	MSG_DIST_TYPE_EVERYONE_STRIPPED,
	MSG_DIST_TYPE_EVERYONE,
};

struct MsgStartCreateInfo
{
	uint32_t lp;
	std::size_t t0DSz;  // Team 0 Deck size
	std::size_t t0EdSz; // Team 0 Extra Deck size
	std::size_t t1DSz;  // Team 1 Deck size
	std::size_t t1EdSz; // Team 1 Extra Deck size
};

struct QuerySingleRequest
{
	uint8_t con;
	uint32_t loc;
	uint32_t seq;
	uint32_t flags;
};

struct QueryLocationRequest
{
	uint8_t con;
	uint32_t loc;
	uint32_t flags;
};

struct Query
{
	uint32_t flags;
	uint32_t code;
	uint32_t pos;
	uint32_t alias;
	uint32_t type;
	uint32_t level;
	uint32_t rank;
	uint32_t link;
	uint32_t attribute;
	uint32_t race;
	int32_t attack;
	int32_t defense;
	int32_t bAttack;
	int32_t bDefense;
	uint32_t reason;
	uint8_t owner;
	uint32_t status;
	uint8_t isPublic;
	uint32_t lscale;
	uint32_t rscale;
	uint32_t linkMarker;
	LocInfo reasonCard;
	LocInfo equipCard;
	uint8_t isHidden;
	uint32_t cover;
	std::vector<LocInfo> targets;
	std::vector<uint32_t> overlays;
	std::vector<uint32_t> counters;
};

using Buffer = std::vector<uint8_t>;
using Msg = std::vector<uint8_t>;
using QueryBuffer = std::vector<uint8_t>;
using QueryRequest = std::variant<QuerySingleRequest, QueryLocationRequest>;
using QueryOpt = std::optional<Query>;
using QueryOptVector = std::vector<QueryOpt>;

// Takes the buffer you would get from OCG_DuelGetMessage and splits it
// into individual core messages (which are still just buffers).
// This operation also removes the length bytes (first 2 bytes) as that
// can be retrieved back from Msg's size() method.
std::vector<Msg> SplitToMsgs(const Buffer& buffer) noexcept;

// Takes any core message, reads and returns its type (1st byte)
uint8_t GetMessageType(const Msg& msg) noexcept;

// Tells if the message requires an answer (setting a response)
// from a user/duelist before processing can continue.
bool DoesMessageRequireAnswer(uint8_t msgType) noexcept;

// Takes any core message and determines how the message should be
// distributed to clients and if it should have knowledge stripped.
MsgDistType GetMessageDistributionType(const Msg& msg) noexcept;

// Tells which team should receive this message.
// The behavior is undefined if the message is not for a specific team.
uint8_t GetMessageReceivingTeam(const Msg& msg) noexcept;

// Removes knowledge from a message if it shouldn't be known
// by the argument `team`, returns a new copy of the message, modified.
Msg StripMessageForTeam(uint8_t team, Msg msg) noexcept;

// Creates MSG_START, which is the first message recorded onto the replay
// and the first one sent to clients, it setups the piles with the correct
// amount of cards and sets the LP to the correct amount.
Msg MakeStartMsg(const MsgStartCreateInfo& info) noexcept;

// The following functions process the message and acquires the query requests
// that are necessary either before distribution or after, respectively.
std::vector<QueryRequest> GetPreDistQueryRequests(const Msg& msg) noexcept;
std::vector<QueryRequest> GetPostDistQueryRequests(const Msg& msg) noexcept;

// Creates MSG_UPDATE_CARD, which is a message that wraps around a single card
// query from a duel.
Msg MakeUpdateCardMsg(uint8_t con, uint32_t loc, uint32_t seq, const QueryBuffer& qb) noexcept;

// Creates MSG_UPDATE_DATA, which is a message that wraps around queries
// from a duel.
Msg MakeUpdateDataMsg(uint8_t con, uint32_t loc, const QueryBuffer& qb) noexcept;

// Creates a query object which is populated with the information from the
// passed QueryBuffer.
QueryOpt DeserializeSingleQueryBuffer(const QueryBuffer& qb) noexcept;

// Creates a vector with multiple query objects which are populated with
// the information from the passed QueryBuffer.
QueryOptVector DeserializeLocationQueryBuffer(const QueryBuffer& qb) noexcept;

// Creates a QueryBuffer which might have information stripped if it had the
// hidden flag set, aditionally, that flag can be overriden with isPublic.
QueryBuffer SerializeSingleQuery(const QueryOpt& qOpt, bool isPublic) noexcept;

// Same as the above function, but for all the queries in the vector.
QueryBuffer SerializeLocationQuery(const QueryOptVector& qs, bool isPublic) noexcept;

} // namespace YGOPro::CoreUtils

#endif // YGOPRO_COREUTILS_HPP
