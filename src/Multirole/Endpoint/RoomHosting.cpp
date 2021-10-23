#include "RoomHosting.hpp"

#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include "../I18N.hpp"
#include "../Lobby.hpp"
#include "../STOCMsgFactory.hpp"
#include "../Workaround.hpp"
#include "../Room/Client.hpp"
#include "../Room/Instance.hpp"
#include "../Service/BanlistProvider.hpp"
#include "../YGOPro/Config.hpp"
#include "../YGOPro/CTOSMsg.hpp"
#include "../YGOPro/StringUtils.hpp"

namespace Ignis::Multirole::Endpoint
{

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
		l.extra.max = 12;
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
		l.extra.max = 6;
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

class RoomHosting::Connection final : public std::enable_shared_from_this<Connection>
{
public:
	Connection(const RoomHosting& roomHosting,
		boost::asio::ip::tcp::socket socket) noexcept
		:
		roomHosting(roomHosting),
		socket(std::move(socket))
	{}

	void DoReadHeader() noexcept
	{
		auto self(shared_from_this());
		auto buffer = boost::asio::buffer(incoming.Data(), YGOPro::CTOSMsg::HEADER_LENGTH);
		boost::asio::async_read(socket, buffer,
		[this, self](boost::system::error_code ec, std::size_t /*unused*/)
		{
			if(!ec && incoming.IsHeaderValid())
				DoReadBody();
		});
	}
private:
	enum class Status
	{
		STATUS_CONTINUE,
		STATUS_MOVED,
		STATUS_ERROR,
	};

	const RoomHosting& roomHosting;
	boost::asio::ip::tcp::socket socket;
	std::string ip;
	std::string name;
	YGOPro::CTOSMsg incoming;
	std::queue<YGOPro::STOCMsg> outgoing;

	void DoReadBody() noexcept
	{
		auto self(shared_from_this());
		auto buffer = boost::asio::buffer(incoming.Body(), incoming.GetLength());
		boost::asio::async_read(socket, buffer,
		[this, self](boost::system::error_code ec, std::size_t /*unused*/)
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

	void DoWrite() noexcept
	{
		assert(!outgoing.empty());
		auto self(shared_from_this());
		const auto& front = outgoing.front();
		boost::asio::async_write(socket, boost::asio::buffer(front.Data(), front.Length()),
		[this, self](boost::system::error_code ec, std::size_t /*unused*/)
		{
			if(ec)
				return;
			outgoing.pop();
			if(!outgoing.empty())
				DoWrite();
			else
				socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		});
	}

	void DoReadEnd() noexcept
	{
		auto self(shared_from_this());
		auto buffer = boost::asio::buffer(incoming.Data(), YGOPro::CTOSMsg::MSG_MAX_LENGTH);
		socket.async_read_some(buffer,
		[this, self](boost::system::error_code ec, std::size_t /*unused*/)
		{
			if(!ec)
				DoReadEnd();
		});
	}

	Status HandleMsg() noexcept
	{
		auto PushToWriteQueue = [&](PrebuiltMsgId id)
		{
			assert(id >= PrebuiltMsgId::PREBUILT_MSG_VERSION_MISMATCH);
			assert(id < PrebuiltMsgId::PREBUILT_MSG_COUNT);
			outgoing.push(roomHosting.prebuiltMsgs[static_cast<std::size_t>(id)]);
		};
		switch(incoming.GetType())
		{
		case YGOPro::CTOSMsg::MsgType::PLAYER_INFO:
		{
			boost::system::error_code ec;
			const auto endpoint = socket.remote_endpoint(ec);
			if(ec) // Cant figure out the address, drop connection.
			{
				PushToWriteQueue(PrebuiltMsgId::PREBUILT_CANNOT_RESOLVE_IP);
				PushToWriteQueue(PrebuiltMsgId::PREBUILT_GENERIC_JOIN_ERROR);
				return Status::STATUS_ERROR;
			}
			ip = endpoint.address().to_string();
			if(roomHosting.lobby.HasMaxConnections(ip))
			{
				PushToWriteQueue(PrebuiltMsgId::PREBUILT_MAX_CONNECTION_REACHED);
				PushToWriteQueue(PrebuiltMsgId::PREBUILT_GENERIC_JOIN_ERROR);
				return Status::STATUS_ERROR;
			}
			const auto p = incoming.GetPlayerInfo();
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
			// All the info required to construct a working room is set here.
			Room::Instance::CreateInfo info
			{
				roomHosting.ioCtx,
				std::string(p->notes),
				Utf16BufferToStr(p->pass),
				roomHosting.svc,
				0U, // NOTE: id, set by lobby.
				{{}}, // NOTE: seed, set by lobby.
				roomHosting.svc.banlistProvider.GetBanlistByHash(p->hostInfo.banlistHash),
				p->hostInfo,
				LimitsFromFlags(p->hostInfo.extraRules)
			};
			// Fix some of the options back into expected values in case of
			// exceptions.
			auto& hi = info.hostInfo;
			if(info.banlist == nullptr)
				hi.banlistHash = 0U;
			hi.t0Count = std::clamp(hi.t0Count, 1, 3);
			hi.t1Count = std::clamp(hi.t1Count, 1, 3);
			hi.bestOf = std::max(hi.bestOf, 1);
			// Remove Relay flag if its 1v1.
			constexpr uint32_t DUEL_RELAY = 0x80;
			bool isRelay = (hi.duelFlagsLow & DUEL_RELAY) != 0U;
			if(isRelay && hi.t0Count == 1U && hi.t1Count == 1U)
			{
				isRelay = false;
				hi.duelFlagsLow &= ~DUEL_RELAY;
			}
			// Deduce LP if its set to 0.
			if(hi.startingLP == 0U)
			{
				if((hi.t0Count == 1U && hi.t1Count == 1U) || isRelay)
				{
					hi.startingLP = 8000U;
				}
				else // Tag mode.
				{
					hi.startingLP = std::max(hi.t0Count, hi.t1Count) * 8000U;
				}
			}
			// Add flag that client should be setting.
			// NOLINTNEXTLINE: DUEL_PSEUDO_SHUFFLE
			hi.duelFlagsLow |= (!hi.dontShuffleDeck) ? 0x0 : 0x10;
			// Let the lobby make the room so we can list it later.
			auto room = roomHosting.lobby.MakeRoom(info);
			// Add the client to the newly created room.
			std::make_shared<Room::Client>(
				roomHosting.lobby,
				std::move(room),
				std::move(socket),
				std::move(ip),
				std::move(name))->Start();
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
			auto room = roomHosting.lobby.GetRoomById(p->id);
			if(!room)
			{
				PushToWriteQueue(PrebuiltMsgId::PREBUILT_ROOM_NOT_FOUND);
				PushToWriteQueue(PrebuiltMsgId::PREBUILT_GENERIC_JOIN_ERROR);
				return Status::STATUS_ERROR;
			}
			if(!room->CheckPassword(Utf16BufferToStr(p->pass)))
			{
				PushToWriteQueue(PrebuiltMsgId::PREBUILT_ROOM_WRONG_PASS);
				return Status::STATUS_ERROR;
			}
			if(room->CheckKicked(ip))
			{
				PushToWriteQueue(PrebuiltMsgId::PREBUILT_KICKED_BEFORE);
				PushToWriteQueue(PrebuiltMsgId::PREBUILT_GENERIC_JOIN_ERROR);
				return Status::STATUS_ERROR;
			}
			std::make_shared<Room::Client>(
				roomHosting.lobby,
				std::move(room),
				std::move(socket),
				std::move(ip),
				std::move(name))->Start();
			return Status::STATUS_MOVED;
		}
		default:
		{
			PushToWriteQueue(PrebuiltMsgId::PREBUILT_INVALID_MSG);
			PushToWriteQueue(PrebuiltMsgId::PREBUILT_GENERIC_JOIN_ERROR);
			return Status::STATUS_ERROR;
		}
		}
	}
};

inline YGOPro::STOCMsg SrvMsg(const char* const str)
{
	return STOCMsgFactory::MakeChat(CHAT_MSG_TYPE_ERROR, str);
}

// public

RoomHosting::RoomHosting(boost::asio::io_context& ioCtx, Service& svc, Lobby& lobby, unsigned short port)
	:
	prebuiltMsgs({
		STOCMsgFactory::MakeVersionError(YGOPro::SERVER_VERSION),
		SrvMsg(I18N::CLIENT_ROOM_HOSTING_INVALID_NAME),
		SrvMsg(I18N::CLIENT_ROOM_HOSTING_NOT_FOUND),
		STOCMsgFactory::MakeJoinError(Error::JOIN_WRONG_PASS),
		SrvMsg(I18N::CLIENT_ROOM_HOSTING_INVALID_MSG),
		STOCMsgFactory::MakeJoinError(Error::JOIN_NOT_FOUND),
		SrvMsg(I18N::CLIENT_ROOM_HOSTING_KICKED_BEFORE),
		SrvMsg(I18N::CLIENT_ROOM_HOSTING_CANNOT_RESOLVE_IP),
		SrvMsg(I18N::CLIENT_ROOM_HOSTING_MAX_CONNECTION_REACHED),
	}),
	ioCtx(ioCtx),
	svc(svc),
	lobby(lobby),
	acceptor(ioCtx, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), port))
{
	Workaround::SetCloseOnExec(acceptor.native_handle());
	acceptor.set_option(boost::asio::socket_base::keep_alive(true));
	DoAccept();
}

void RoomHosting::Stop() noexcept
{
	acceptor.close();
}

// private

void RoomHosting::DoAccept()
{
	acceptor.async_accept(
	[this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket)
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

} // namespace Ignis::Multirole::Endpoint
