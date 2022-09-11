#ifndef STOCMSGFACTORY_HPP
#define STOCMSGFACTORY_HPP
#include "Room/Client.hpp"
#include "YGOPro/STOCMsg.hpp"

namespace Ignis::Multirole
{

namespace Error
{

enum Join : uint32_t
{
	JOIN_WRONG_PASS = 0x1,
// 	JOIN_REFUSED    = 0x2,
	JOIN_NOT_FOUND  = 0x9,
};

enum DeckOrCard : uint8_t
{
	DECK_BAD_MAIN_COUNT   = 0x6,
	DECK_BAD_EXTRA_COUNT  = 0x7,
	DECK_BAD_SIDE_COUNT   = 0x8,
	DECK_INVALID_SIZE     = 0xB,
	DECK_TOO_MANY_LEGENDS = 0xC,
	DECK_TOO_MANY_SKILLS  = 0xD,
	CARD_BANLISTED        = 0x1,
	CARD_OCG_ONLY         = 0x2,
	CARD_TCG_ONLY         = 0x3,
	CARD_UNKNOWN          = 0x4,
	CARD_MORE_THAN_3      = 0x5,
	CARD_UNOFFICIAL       = 0xA,
	CARD_FORBIDDEN_TYPE   = 0x9,
};

} // namespace Error

enum ChatMsgType : uint8_t
{
	CHAT_MSG_TYPE_INFO,
	CHAT_MSG_TYPE_ERROR,
	CHAT_MSG_TYPE_SHOUT,
};

enum PChangeType : uint8_t
{
	PCHANGE_TYPE_SPECTATE  = 0x8,
	PCHANGE_TYPE_READY     = 0x9,
	PCHANGE_TYPE_NOT_READY = 0xA,
	PCHANGE_TYPE_LEAVE     = 0xB,
};

class STOCMsgFactory
{
public:
	STOCMsgFactory(uint8_t t1max);

	YGOPro::STOCMsg MakeTypeChange(const Room::Client& c, bool isHost) const;

	// Creates chat message from client (client message)
	static YGOPro::STOCMsg MakeChat(const Room::Client& c, bool isTeam, std::string_view str);
	// Creates chat message from specific type (system message)
	static YGOPro::STOCMsg MakeChat(ChatMsgType type, std::string_view str);
	// Creates a message used to inform players entering the room
	YGOPro::STOCMsg MakePlayerEnter(const Room::Client& c) const;
	// Creates client Ready status change
	YGOPro::STOCMsg MakePlayerChange(const Room::Client& c) const;
	// Creates client status change based on PChangeType
	YGOPro::STOCMsg MakePlayerChange(const Room::Client& c, PChangeType pc) const;
	// Creates message that moves a player from one position to another
	YGOPro::STOCMsg MakePlayerChange(Room::Client::PosType p1, Room::Client::PosType p2) const;
	// Creates a message that updates spectator count for clients
	static YGOPro::STOCMsg MakeWatchChange(std::size_t count);
	// Creates a message that signals the client the duel is starting
	static YGOPro::STOCMsg MakeDuelStart();
	// Creates a message signaling client that the room lifetime ended
	static YGOPro::STOCMsg MakeDuelEnd();
	// Signals clients to do a Rock Paper Scissor choice
	static YGOPro::STOCMsg MakeAskRPS();
	// Creates a message to ask a client if he desires to go first or not
	static YGOPro::STOCMsg MakeAskIfGoingFirst();
	// Creates a message that has both teams RPS face-off result
	static YGOPro::STOCMsg MakeRPSResult(uint8_t t0, uint8_t t1);
	// Creates a message that wraps around a core message
	static YGOPro::STOCMsg MakeGameMsg(const std::vector<uint8_t>& msg);
	// Creates a message to ask a client if he desires to rematch
	static YGOPro::STOCMsg MakeAskIfRematch();
	// Creates a message signaling client to wait for rematch answers
	static YGOPro::STOCMsg MakeRematchWait();
	// Creates a message signaling the client to perform sidedecking
	static YGOPro::STOCMsg MakeAskSidedeck();
	// Creates a message that tells clients to wait for sidedecking
	static YGOPro::STOCMsg MakeSidedeckWait();
	// Creates a message that tells clients to catch up the following
	// next messages
	static YGOPro::STOCMsg MakeCatchUp(bool catchingUp);
	// Creates a message that tells clients how much time a particular
	// team has left
	static YGOPro::STOCMsg MakeTimeLimit(uint8_t team, uint16_t timeLeft);
	// Creates a message that wraps around a replay file to send to clients
	static YGOPro::STOCMsg MakeSendReplay(const std::vector<uint8_t>& bytes);
	// Creates a message that tells the client to save the sent replay
	static YGOPro::STOCMsg MakeOpenReplayPrompt();

	// Error messages
	static YGOPro::STOCMsg MakeJoinError(Error::Join type);
	static YGOPro::STOCMsg MakeDeckError(Error::DeckOrCard type, uint32_t code);
	static YGOPro::STOCMsg MakeDeckError(
		Error::DeckOrCard type,
		std::size_t got,
		std::size_t min,
		std::size_t max);
	static YGOPro::STOCMsg MakeVersionError(const YGOPro::ClientVersion& ver);
	static YGOPro::STOCMsg MakeSideError();
protected:
	uint8_t EncodePosition(Room::Client::PosType pos) const;

private:
	const uint8_t t1max;
};

} // namespace Ignis::Multirole

#endif // STOCMSGFACTORY_HPP
