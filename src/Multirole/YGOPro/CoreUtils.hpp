#ifndef YGOPRO_COREUTILS_HPP
#define YGOPRO_COREUTILS_HPP
#include <cstdint>
#include <vector>
#include <variant>

#include "STOCMsg.hpp"

namespace YGOPro::CoreUtils
{

using Buffer = std::vector<uint8_t>;

using Msg = std::vector<uint8_t>;

using StrippedMsg = std::variant<const Msg*, Msg>;

enum class MsgDistType
{
	MSG_DIST_TYPE_FOR_EVERYONE,
	MSG_DIST_TYPE_FOR_EVERYONE_WITHOUT_STRIPPING,
	MSG_DIST_TYPE_FOR_SPECIFIC_TEAM,
	MSG_DIST_TYPE_FOR_SPECIFIC_TEAM_DUELIST,
	MSG_DIST_TYPE_FOR_EVERYONE_EXCEPT_TEAM_DUELIST,
};

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

// Takes any core message and determines how the message should be
// distributed to clients and if it shouldn't be stripped from knowledge.
// Throws std::out_of_range if msg's size is unexpectedly short.
MsgDistType GetMessageDistributionType(const Msg& msg);

// Tells which team should receive this message.
// The behavior is undefined if the message is not for a specific team.
// Throws std::out_of_range if msg's size is less than 2 (type + team).
uint8_t GetMessageReceivingTeam(const Msg& msg);

// Removes knowledge from a message if it shouldn't be known
// by the argument `team`. If the message doesn't require knowledge
// stripping then the returned variant will hold a pointer to the passed
// msg, else it will hold a new copy of the message with knowledge stripped.
StrippedMsg StripMessageForTeam(uint8_t team, const Msg& msg);

// Shortcut, self explanatory.
const Msg& MsgFromStrippedMsg(const StrippedMsg& sMsg);

// Creates a game message ready to be sent to a client from a core message.
STOCMsg GameMsgFromMsg(const Msg& msg);

} // namespace YGOPro::CoreUtils

#endif // YGOPRO_COREUTILS_HPP
