#include "RoomHosting.hpp"

#include <asio/read.hpp>
#include <asio/write.hpp>

#include "../BanlistProvider.hpp"
#include "../DataProvider.hpp"
#include "../Lobby.hpp"
#include "../STOCMsgFactory.hpp"
#include "../Workaround.hpp"
#include "../Room/Client.hpp"
#include "../Room/Instance.hpp"
#include "../YGOPro/Config.hpp"
#include "../YGOPro/StringUtils.hpp"

namespace Ignis::Multirole::Endpoint
{

constexpr const char* ROOM_404 = "Room not found. Try refreshing the list!";
constexpr const char* INVALID_NAME = "Invalid name. Try filling in your name.";
constexpr const char* INVALID_MSG =
"Invalid message before connecting to room. Please report this error!";

constexpr bool operator!=(
	const YGOPro::ClientVersion& v1,
	const YGOPro::ClientVersion& v2)
{
	return std::tie(
		v1.client.major,
		v1.client.minor,
		v1.core.major,
		v1.core.minor)
		!=
		std::tie(
		v2.client.major,
		v2.client.minor,
		v2.core.major,
		v2.core.minor);
}

constexpr YGOPro::DeckLimits LimitsFromFlags(uint16_t flag)
{
	const bool doubleDeck = (flag & YGOPro::EXTRA_RULE_DOUBLE_DECK) != 0;
	const bool limit20 = (flag & YGOPro::EXTRA_RULE_DECK_LIMIT_20) != 0;
	YGOPro::DeckLimits l; // initialized with official values
	if(doubleDeck && limit20)
	{
		// NOTE: main deck boundaries are same as official
		l.extra.max = 10;
		l.side.max = 12;
	}
	else if(doubleDeck)
	{
		l.main.min = l.main.max = 100;
		l.extra.max = 30;
		l.side.max = 30;
	}
	else if(limit20)
	{
		l.main.min = 20; l.main.max = 30;
		l.extra.max = 5;
		l.side.max = 6;
	}
	return l;
}

template<typename Buffer>
inline std::string Utf16BufferToStr(const Buffer& buffer)
{
	using namespace YGOPro;
	return UTF16ToUTF8(BufferToUTF16(buffer, sizeof(Buffer)));
}

// public

RoomHosting::RoomHosting(CreateInfo&& info)
	:
	prebuiltMsgs({
		STOCMsgFactory::MakeVersionError(YGOPro::SERVER_VERSION),
		STOCMsgFactory::MakeChat(CHAT_MSG_TYPE_ERROR, INVALID_NAME),
		STOCMsgFactory::MakeChat(CHAT_MSG_TYPE_ERROR, ROOM_404),
		STOCMsgFactory::MakeJoinError(Error::JOIN_WRONG_PASS),
		STOCMsgFactory::MakeChat(CHAT_MSG_TYPE_ERROR, INVALID_MSG),
		STOCMsgFactory::MakeJoinError(Error::JOIN_NOT_FOUND),
	}),
	ioCtx(info.ioCtx),
	acceptor(info.ioCtx, asio::ip::tcp::endpoint(asio::ip::tcp::v6(), info.port)),
	banlistProvider(info.banlistProvider),
	coreProvider(info.coreProvider),
	dataProvider(info.dataProvider),
	replayManager(info.replayManager),
	scriptProvider(info.scriptProvider),
	lobby(info.lobby)
{
	Workaround::SetCloseOnExec(acceptor.native_handle());
	DoAccept();
}

void RoomHosting::Stop()
{
	acceptor.close();
}

const YGOPro::STOCMsg& RoomHosting::GetPrebuiltMsg(PrebuiltMsgId id) const
{
	assert(id >= PrebuiltMsgId::PREBUILT_MSG_VERSION_MISMATCH);
	assert(id < PrebuiltMsgId::PREBUILT_MSG_COUNT);
	return prebuiltMsgs[static_cast<std::size_t>(id)];
}

std::shared_ptr<Room::Instance> RoomHosting::GetRoomById(uint32_t id) const
{
	return lobby.GetRoomById(id);
}

Room::Instance::CreateInfo RoomHosting::GetBaseRoomCreateInfo(uint32_t banlistHash) const
{
	return Room::Instance::CreateInfo
	{
		lobby,
		ioCtx,
		coreProvider,
		replayManager,
		scriptProvider,
		dataProvider.GetDatabase(),
		{}, // hostInfo
		{}, // limits
		banlistProvider.GetBanlistByHash(banlistHash),
		{}, // name
		{}, // notes
		{}  // password
	};
}

// private

void RoomHosting::DoAccept()
{
	acceptor.async_accept(
	[this](const std::error_code& ec, asio::ip::tcp::socket socket)
	{
		if(!acceptor.is_open())
			return;
		if(!ec)
		{
			Workaround::SetCloseOnExec(socket.native_handle());
			std::make_shared<Connection>(*this, std::move(socket))->DoReadHeader();
		}
		DoAccept();
	});
}

RoomHosting::Connection::Connection(
	const RoomHosting& roomHosting,
	asio::ip::tcp::socket socket)
	:
	roomHosting(roomHosting),
	socket(std::move(socket)),
	name(),
	incoming(),
	outgoing()
{}


void RoomHosting::Connection::DoReadHeader()
{
	auto self(shared_from_this());
	auto buffer = asio::buffer(incoming.Data(), YGOPro::CTOSMsg::HEADER_LENGTH);
	asio::async_read(socket, buffer,
	[this, self](std::error_code ec, std::size_t /*unused*/)
	{
		if(!ec && incoming.IsHeaderValid())
			DoReadBody();
	});
}

void RoomHosting::Connection::DoReadBody()
{
	auto self(shared_from_this());
	auto buffer = asio::buffer(incoming.Body(), incoming.GetLength());
	asio::async_read(socket, buffer,
	[this, self](std::error_code ec, std::size_t /*unused*/)
	{
		if(ec)
			return;
		if(const auto status = HandleMsg(); status == Status::STATUS_CONTINUE)
		{
			DoReadHeader();
		}
		else if(status == Status::STATUS_ERROR)
		{
			DoReadEnd();
			DoWrite();
		}
	});
}

void RoomHosting::Connection::DoReadEnd()
{
	auto self(shared_from_this());
	auto buffer = asio::buffer(incoming.Data(), YGOPro::CTOSMsg::MSG_MAX_LENGTH);
	socket.async_read_some(buffer,
	[this, self](std::error_code ec, std::size_t /*unused*/)
	{
		if(!ec)
			DoReadEnd();
	});
}

void RoomHosting::Connection::DoWrite()
{
	assert(!outgoing.empty());
	auto self(shared_from_this());
	const auto& front = outgoing.front();
	asio::async_write(socket, asio::buffer(front.Data(), front.Length()),
	[this, self](std::error_code ec, std::size_t /*unused*/)
	{
		if(ec)
			return;
		outgoing.pop();
		if(!outgoing.empty())
			DoWrite();
		else
			socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
	});
}

RoomHosting::Connection::Status RoomHosting::Connection::HandleMsg()
{
	auto PushToWriteQueue = [this](RoomHosting::PrebuiltMsgId id)
	{
		outgoing.push(roomHosting.GetPrebuiltMsg(id));
	};
	switch(incoming.GetType())
	{
	case YGOPro::CTOSMsg::MsgType::PLAYER_INFO:
	{
		auto p = incoming.GetPlayerInfo();
		if(!p || (name = Utf16BufferToStr(p->name)).empty())
		{
			PushToWriteQueue(PrebuiltMsgId::PREBUILT_INVALID_NAME);
			PushToWriteQueue(PrebuiltMsgId::PREBUILT_GENERIC_JOIN_ERROR);
			return Status::STATUS_ERROR;
		}
		return Status::STATUS_CONTINUE;
	}
	case YGOPro::CTOSMsg::MsgType::CREATE_GAME:
	{
		auto p = incoming.GetCreateGame();
		if(!p || p->hostInfo.handshake != YGOPro::SERVER_HANDSHAKE ||
		   p->hostInfo.version != YGOPro::SERVER_VERSION)
		{
			PushToWriteQueue(PrebuiltMsgId::PREBUILT_MSG_VERSION_MISMATCH);
			return Status::STATUS_ERROR;
		}
		*std::rbegin(p->notes) = '\0'; // Guarantee null-terminated string.
		// Get "template" information that will be modified and used.
		auto info = roomHosting.GetBaseRoomCreateInfo(p->hostInfo.banlistHash);
		// Set our custom info.
		info.hostInfo = p->hostInfo;
		info.limits = LimitsFromFlags(info.hostInfo.extraRules);
		info.name = Utf16BufferToStr(p->name);
		info.notes = std::string(p->notes);
		info.pass = Utf16BufferToStr(p->pass);
		// Fix some of the options back into expected values in case of
		// exceptions.
		auto& hi = info.hostInfo;
		if(info.banlist == nullptr)
			hi.banlistHash = 0U;
		hi.t0Count = std::clamp(hi.t0Count, 1, 3);
		hi.t1Count = std::clamp(hi.t1Count, 1, 3);
		hi.bestOf = std::max(hi.bestOf, 1);
		// Add flag that client should be setting.
		// NOLINTNEXTLINE: DUEL_PSEUDO_SHUFFLE
		hi.duelFlagsLow |= (!hi.dontShuffleDeck) ? 0x0 : 0x10;
		// Make new room with the set parameters
		auto room = std::make_shared<Room::Instance>(std::move(info));
		room->RegisterToOwner();
		// Add the client to the newly created room
		auto client = std::make_shared<Room::Client>(
			room,
			std::move(socket),
			std::move(name));
		client->RegisterToOwner();
		client->Start();
		return Status::STATUS_MOVED;
	}
	case YGOPro::CTOSMsg::MsgType::JOIN_GAME:
	{
		auto p = incoming.GetJoinGame();
		if(!p || p->version != YGOPro::SERVER_VERSION)
		{
			PushToWriteQueue(PrebuiltMsgId::PREBUILT_MSG_VERSION_MISMATCH);
			return Status::STATUS_ERROR;
		}
		if(auto room = roomHosting.GetRoomById(p->id); !room)
		{
			PushToWriteQueue(PrebuiltMsgId::PREBUILT_ROOM_NOT_FOUND);
			PushToWriteQueue(PrebuiltMsgId::PREBUILT_GENERIC_JOIN_ERROR);
			return Status::STATUS_ERROR;
		}
		else if(!room->CheckPassword(Utf16BufferToStr(p->pass)))
		{
			PushToWriteQueue(PrebuiltMsgId::PREBUILT_ROOM_WRONG_PASS);
			return Status::STATUS_ERROR;
		}
		else
		{
			auto client = std::make_shared<Room::Client>(
				room,
				std::move(socket),
				std::move(name));
			client->RegisterToOwner();
			client->Start();
			return Status::STATUS_MOVED;
		}
	}
	default:
	{
		PushToWriteQueue(PrebuiltMsgId::PREBUILT_INVALID_MSG);
		PushToWriteQueue(PrebuiltMsgId::PREBUILT_GENERIC_JOIN_ERROR);
		return Status::STATUS_ERROR;
	}
	}
}

} // namespace Ignis::Multirole::Endpoint
