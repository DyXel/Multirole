#include "STOCMsgFactory.hpp"

#include "YGOPro/StringUtils.hpp"

namespace Ignis::Multirole
{

using namespace YGOPro;

// public

STOCMsgFactory::STOCMsgFactory(uint8_t t1max) : t1max(t1max)
{}

STOCMsg STOCMsgFactory::MakeTypeChange(const Room::Client& c, bool isHost) const
{
	const auto posKey = c.Position();
	uint8_t pos = 0U;
	if(posKey == Room::Client::POSITION_SPECTATOR)
		pos = 7U; // NOLINT: Sum of both teams max player + 1
	else
		pos = EncodePosition(posKey);
	STOCMsg::TypeChange proto{};
	proto.type = (static_cast<uint8_t>(isHost) << 4U) | pos;
	return {proto};
}

STOCMsg STOCMsgFactory::MakeChat(const Room::Client::PosType& p, std::string_view str) const
{
	return MakeChat(EncodePosition(p), UTF8ToUTF16(str));
}

STOCMsg STOCMsgFactory::MakeChat(ChatMsgType type, std::string_view str)
{
	auto ChatTypeToPosOrType = [](ChatMsgType type) constexpr -> uint16_t
	{
		switch(type)
		{
		case CHAT_MSG_TYPE_SPECTATOR:
		{
			return 10U;
			break;
		}
		case CHAT_MSG_TYPE_SYSTEM:
		{
			return 8U;
			break;
		}
		case CHAT_MSG_TYPE_ERROR:
		default:
		{
			return 9U;
			break;
		}
		}
	};
	return MakeChat(ChatTypeToPosOrType(type), UTF8ToUTF16(str));
}

STOCMsg STOCMsgFactory::MakePlayerEnter(const Room::Client& c) const
{
	const auto& str16 = UTF8ToUTF16(c.Name());
	STOCMsg::PlayerEnter proto{};
	std::memcpy(&proto.name, str16.data(), UTF16ByteCount(str16));
	proto.pos = EncodePosition(c.Position());
	return {proto};
}

STOCMsg STOCMsgFactory::MakePlayerChange(const Room::Client& c) const
{
	STOCMsg::PlayerChange proto{};
	const uint8_t pos = EncodePosition(c.Position());
	PChangeType pct = (c.Ready()) ? PCHANGE_TYPE_READY : PCHANGE_TYPE_NOT_READY;
	proto.status = (pos << 4U) | static_cast<uint8_t>(pct);
	return {proto};
}

STOCMsg STOCMsgFactory::MakePlayerChange(const Room::Client& c, PChangeType pct) const
{
	STOCMsg::PlayerChange proto{};
	const uint8_t pos = EncodePosition(c.Position());
	proto.status = (pos << 4U) | static_cast<uint8_t>(pct);
	return {proto};
}

STOCMsg STOCMsgFactory::MakePlayerChange(Room::Client::PosType p1, Room::Client::PosType p2) const
{
	STOCMsg::PlayerChange proto{};
	const uint8_t pos1 = EncodePosition(p1);
	const uint8_t pos2 = EncodePosition(p2);
	proto.status = (pos1 << 4U) | pos2;
	return {proto};
}

STOCMsg STOCMsgFactory::MakeWatchChange(std::size_t count)
{
	return {STOCMsg::WatchChange{static_cast<uint16_t>(count)}};
}

STOCMsg STOCMsgFactory::MakeDuelStart()
{
	return {STOCMsg::MsgType::DUEL_START};
}

STOCMsg STOCMsgFactory::MakeDuelEnd()
{
	return {STOCMsg::MsgType::DUEL_END};
}

STOCMsg STOCMsgFactory::MakeAskRPS()
{
	return {STOCMsg::MsgType::CHOOSE_RPS};
}

STOCMsg STOCMsgFactory::MakeAskIfGoingFirst()
{
	return {STOCMsg::MsgType::CHOOSE_ORDER};
}

STOCMsg STOCMsgFactory::MakeRPSResult(uint8_t t0, uint8_t t1)
{
	return {STOCMsg::RPSResult{t0, t1}};
}

STOCMsg STOCMsgFactory::MakeGameMsg(const std::vector<uint8_t>& msg)
{
	return STOCMsg{STOCMsg::MsgType::GAME_MSG, msg};
}

STOCMsg STOCMsgFactory::MakeAskIfRematch()
{
	return {STOCMsg::MsgType::REMATCH};
}

STOCMsg STOCMsgFactory::MakeRematchWait()
{
	return {STOCMsg::MsgType::REMATCH_WAIT};
}

STOCMsg STOCMsgFactory::MakeAskSidedeck()
{
	return {STOCMsg::MsgType::CHANGE_SIDE};
}

STOCMsg STOCMsgFactory::MakeSidedeckWait()
{
	return {STOCMsg::MsgType::WAITING_SIDE};
}

STOCMsg STOCMsgFactory::MakeCatchUp(bool catchingUp)
{
	return {STOCMsg::CatchUp{static_cast<uint8_t>(catchingUp)}};
}

STOCMsg STOCMsgFactory::MakeTimeLimit(uint8_t team, uint16_t timeLeft)
{
	return {STOCMsg::TimeLimit{team, timeLeft}};
}

STOCMsg STOCMsgFactory::MakeJoinError(Error::Join type)
{
	return {STOCMsg::ErrorMsg{1U, static_cast<uint32_t>(type)}};
}

STOCMsg STOCMsgFactory::MakeDeckError(Error::DeckOrCard type, uint32_t code)
{
	return
	{
		STOCMsg::DeckErrorMsg
		{
			2U,
			static_cast<uint32_t>(type),
			{
				0U,
				0U,
				0U
			},
			code
		}
	};
}

STOCMsg STOCMsgFactory::MakeDeckError(
	Error::DeckOrCard type,
	std::size_t got,
	std::size_t min,
	std::size_t max)
{
	return
	{
		STOCMsg::DeckErrorMsg
		{
			2U,
			static_cast<uint32_t>(type),
			{
				static_cast<uint32_t>(got),
				static_cast<uint32_t>(min),
				static_cast<uint32_t>(max),
			},
			0U
		}
	};
}

STOCMsg STOCMsgFactory::MakeVersionError(const ClientVersion& version)
{
	return {STOCMsg::VerErrorMsg{5U, version}};
}

STOCMsg STOCMsgFactory::MakeSideError()
{
	return {STOCMsg::ErrorMsg{3U, 0U}};
}

// protected

uint8_t STOCMsgFactory::EncodePosition(Room::Client::PosType pos) const
{
	return (pos.first * t1max) + pos.second;
}

// private

STOCMsg STOCMsgFactory::MakeChat(uint16_t posOrType, std::u16string_view str16)
{
	const std::size_t str16bc = UTF16ByteCount(str16);
	std::vector<uint8_t> bytes(sizeof(decltype(posOrType)) + str16bc);
	uint8_t* p = bytes.data();
	std::memcpy(p, &posOrType, sizeof(decltype(posOrType)));
	p += sizeof(decltype(posOrType));
	std::memcpy(p, str16.data(), str16bc);
	return {STOCMsg::MsgType::CHAT, bytes};
}

} // namespace Ignis::Multirole
