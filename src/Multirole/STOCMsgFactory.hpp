#ifndef STOCMSGFACTORY_HPP
#define STOCMSGFACTORY_HPP
#include "Client.hpp"
#include "YGOPro/STOCMsg.hpp"

namespace Ignis::Multirole
{

namespace Error
{

enum Join : uint8_t
{
	JOIN_NOT_FOUND  = 0x0,
	JOIN_WRONG_PASS = 0x1,
// 	JOIN_REFUSED    = 0x2,
};

enum DeckOrCard : uint8_t
{
	DECK_BAD_MAIN_COUNT  = 0x6,
	DECK_BAD_EXTRA_COUNT = 0x7,
	DECK_BAD_SIDE_COUNT  = 0x8,
	CARD_BANLISTED       = 0x1,
	CARD_OCG_ONLY        = 0x2,
	CARD_TCG_ONLY        = 0x3,
	CARD_UNKNOWN         = 0x4,
	CARD_MORE_THAN_3     = 0x5,
	CARD_UNOFFICIAL      = 0xA,
	CARD_FORBIDDEN_TYPE  = 0x9,
};

enum Generic : uint8_t
{
	GENERIC_INVALID_SIDE = 0x3,
	GENERIC_EXPECTED_VER = 0x5,
};

} // namespace Error

enum ChatMsgType : uint8_t
{
	CHAT_MSG_TYPE_SPECTATOR,
	CHAT_MSG_TYPE_SYSTEM,
	CHAT_MSG_TYPE_ERROR,
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

	YGOPro::STOCMsg MakeTypeChange(const Client& c, bool isHost) const;

	// Creates chat message from client position (client message)
	YGOPro::STOCMsg MakeChat(const Client& c, std::string_view str) const;
	// Creates chat message from specific type (system message)
	static YGOPro::STOCMsg MakeChat(ChatMsgType type, std::string_view str);

	YGOPro::STOCMsg MakePlayerEnter(const Client& c) const;

	// Creates client Ready status change
	YGOPro::STOCMsg MakePlayerChange(const Client& c) const;
	// Creates client status change based on PChangeType
	YGOPro::STOCMsg MakePlayerChange(const Client& c, PChangeType pc) const;
	// Creates message that moves a player from one position to another
	YGOPro::STOCMsg MakePlayerChange(Client::PosType p1, Client::PosType p2) const;

	static YGOPro::STOCMsg MakeWatchChange(std::size_t count);

	// Error messages
	static YGOPro::STOCMsg MakeError(Error::Join type);
	static YGOPro::STOCMsg MakeError(Error::DeckOrCard type, uint32_t value);
	static YGOPro::STOCMsg MakeError(Error::Generic type, uint32_t value);
protected:
	uint8_t EncodePosition(Client::PosType pos) const;

private:
	const uint8_t t1max;
};

} // namespace Ignis::Multirole

#endif // STOCMSGFACTORY_HPP
