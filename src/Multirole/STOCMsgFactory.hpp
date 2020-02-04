#ifndef STOCMSGFACTORY_HPP
#define STOCMSGFACTORY_HPP
#include "Client.hpp"
#include "YGOPro/STOCMsg.hpp"

namespace Ignis
{

namespace Multirole
{

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
	YGOPro::STOCMsg MakeChat(ChatMsgType type, std::string_view str) const;

	YGOPro::STOCMsg MakePlayerEnter(const Client& c) const;

	// Creates client Ready status change
	YGOPro::STOCMsg MakePlayerChange(const Client& c) const;
	// Creates client status change based on PChangeType
	YGOPro::STOCMsg MakePlayerChange(const Client& c, PChangeType pc) const;
	// Creates message that moves a player from one position to another
	YGOPro::STOCMsg MakePlayerChange(Client::PosType p1, Client::PosType p2) const;

	YGOPro::STOCMsg MakeWatchChange(std::size_t count) const;
protected:
	uint8_t EncodePosition(Client::PosType pos) const;

private:
	const uint8_t t1max;
};

} // namespace Multirole

} // namespace Ignis

#endif // STOCMSGFACTORY_HPP
