#ifndef YGOPRO_COREUTILS_HPP
#define YGOPRO_COREUTILS_HPP
#include <cstdint>
#include <variant>
#include <vector>

#include "STOCMsg.hpp"

namespace YGOPro::CoreUtils
{

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

using Buffer = std::vector<uint8_t>;
using Msg = std::vector<uint8_t>;
using Query = std::vector<uint8_t>;
using QueryRequest = std::variant<QuerySingleRequest, QueryLocationRequest>;

// Takes the buffer you would get from OCG_DuelGetMessage and splits it
// into individual core messages (which are still just buffers).
// This operation also removes the length bytes (first 2 bytes) as that
// can be retrieved back from Msg's size() method.
// Throws std::out_of_range if any operation would read outside of the
// passed buffer.
std::vector<Msg> SplitToMsgs(const Buffer& buffer);

// Takes any core message, reads and returns its type (1st byte)
// Throws std::out_of_range if msg is empty.
uint8_t GetMessageType(const Msg& msg);

// Tells if the message requires an answer (setting a response)
// from a user/duelist before processing can continue.
bool DoesMessageRequireAnswer(uint8_t msgType);

// Takes any core message and determines how the message should be
// distributed to clients and if it should have knowledge stripped.
// Throws std::out_of_range if msg's size is unexpectedly short.
MsgDistType GetMessageDistributionType(const Msg& msg);

// Tells which team should receive this message.
// The behavior is undefined if the message is not for a specific team.
// Throws std::out_of_range if msg's size is unexpectedly short.
uint8_t GetMessageReceivingTeam(const Msg& msg);

// Removes knowledge from a message if it shouldn't be known
// by the argument `team`, returns a new copy of the message, modified.
Msg StripMessageForTeam(uint8_t team, Msg msg);

// Creates MSG_START, which is the first message recorded onto the replay
// and the first one sent to clients, it setups the piles with the correct
// amount of cards and sets the LP to the correct amount.
Msg MakeStartMsg(const MsgStartCreateInfo& info);

// Creates a game message ready to be sent to a client from a core message.
STOCMsg GameMsgFromMsg(const Msg& msg);

// The following functions process the message and acquires the query requests
// that are necessary either before distribution or after, respectively.
// Throws std::out_of_range if msg's size is unexpectedly short.
std::vector<QueryRequest> GetPreDistQueryRequests(const Msg& msg);
std::vector<QueryRequest> GetPostDistQueryRequests(const Msg& msg);

// Creates MSG_UPDATE_CARD, which is a message that wraps around a single card
// query from a duel.
Msg MakeUpdateCardMsg(uint8_t con, uint32_t loc, uint32_t seq, const Query& q);

// Creates MSG_UPDATE_DATA, which is a message that wraps around queries
// from a duel.
Msg MakeUpdateDataMsg(uint8_t con, uint32_t loc, const Query& q);

} // namespace YGOPro::CoreUtils

#endif // YGOPRO_COREUTILS_HPP
