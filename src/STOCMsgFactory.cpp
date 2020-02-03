#include "STOCMsgFactory.hpp"

#include "StringUtils.hpp"

namespace Ignis
{

namespace Multirole
{

// public

STOCMsgFactory::STOCMsgFactory(uint8_t t1max) : t1max(t1max)
{}

YGOPro::STOCMsg STOCMsgFactory::MakeTypeChange(const Client& c, bool isHost) const
{
	const auto posKey = c.Position();
	uint8_t pos;
	if(posKey == Client::POSITION_SPECTATOR)
		pos = 7; // Sum of both teams max player + 1
	else
		pos = EncodePosition(posKey);
	YGOPro::STOCMsg::TypeChange proto;
	proto.type = (static_cast<uint8_t>(isHost) << 4) | pos;
	return YGOPro::STOCMsg(proto);
}

YGOPro::STOCMsg STOCMsgFactory::MakeChat(const Client& c, std::string_view str) const
{
	using namespace StringUtils;
	YGOPro::STOCMsg::Chat proto{};
	proto.posOrType = EncodePosition(c.Position());
	const auto size = UTF16ToBuffer(proto.msg, UTF8ToUTF16(str));
	return YGOPro::STOCMsg(proto).Shrink(size + sizeof(uint16_t));
}

YGOPro::STOCMsg STOCMsgFactory::MakeChat(ChatMsgType type, std::string_view str) const
{
	using namespace StringUtils;
	YGOPro::STOCMsg::Chat proto{};
	switch(type)
	{
	case CHAT_MSG_TYPE_SPECTATOR:
	{
		proto.posOrType = 10u;
		break;
	}
	case CHAT_MSG_TYPE_SYSTEM:
	{
		proto.posOrType = 8u;
		break;
	}
	case CHAT_MSG_TYPE_ERROR:
	default:
	{
		proto.posOrType = 9u;
		break;
	}
	}
	const auto size = UTF16ToBuffer(proto.msg, UTF8ToUTF16(str));
	return YGOPro::STOCMsg(proto).Shrink(size + sizeof(uint16_t));
}

YGOPro::STOCMsg STOCMsgFactory::MakePlayerEnter(const Client& c) const
{
	YGOPro::STOCMsg::PlayerEnter proto{};
	using namespace StringUtils;
	UTF16ToBuffer(proto.name, UTF8ToUTF16(c.Name()));
	proto.pos = EncodePosition(c.Position());
	return YGOPro::STOCMsg(proto);
}

YGOPro::STOCMsg STOCMsgFactory::MakePlayerChange(const Client& c) const
{
	YGOPro::STOCMsg::PlayerChange proto;
	const uint8_t pos = EncodePosition(c.Position());
	PChangeType pct = (c.Ready()) ? PCHANGE_TYPE_READY : PCHANGE_TYPE_NOT_READY;
	proto.status = (pos << 4) | static_cast<uint8_t>(pct);
	return YGOPro::STOCMsg(proto);
}

YGOPro::STOCMsg STOCMsgFactory::MakePlayerChange(const Client& c, PChangeType pct) const
{
	YGOPro::STOCMsg::PlayerChange proto;
	const uint8_t pos = EncodePosition(c.Position());
	proto.status = (pos << 4) | static_cast<uint8_t>(pct);
	return YGOPro::STOCMsg(proto);
}

YGOPro::STOCMsg STOCMsgFactory::MakePlayerChange(Client::PosType p1, Client::PosType p2) const
{
	YGOPro::STOCMsg::PlayerChange proto;
	const uint8_t pos1 = EncodePosition(p1);
	const uint8_t pos2 = EncodePosition(p2);
	proto.status = (pos1 << 4) | pos2;
	return YGOPro::STOCMsg(proto);
}

YGOPro::STOCMsg STOCMsgFactory::MakeWatchChange(std::size_t count) const
{
	return {YGOPro::STOCMsg::WatchChange{static_cast<uint16_t>(count)}};
}

// protected

uint8_t STOCMsgFactory::EncodePosition(Client::PosType pos) const
{
	return (pos.first * t1max) + pos.second;
}

} // namespace Multirole

} // namespace Ignis
