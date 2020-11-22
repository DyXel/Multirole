#include "RoomHosting.hpp"

#include <type_traits> // std::remove_extent

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

// Holds information about the client before a proper connection to a Room
// has been established.
struct TmpClient
{
	asio::ip::tcp::socket soc;
	YGOPro::CTOSMsg msg;
	std::string name;

	explicit TmpClient(asio::ip::tcp::socket soc) : soc(std::move(soc))
	{}
};

// public

RoomHosting::RoomHosting(CreateInfo&& info)
	:
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
	std::scoped_lock lock(mTmpClients);
	std::error_code ignore;
	for(const auto& c : tmpClients)
	{
		c->soc.shutdown(asio::ip::tcp::socket::shutdown_both, ignore);
		c->soc.close(ignore);
	}
}

// private

void RoomHosting::Add(const std::shared_ptr<TmpClient>& tc)
{
	std::scoped_lock lock(mTmpClients);
	tmpClients.insert(tc);
}

void RoomHosting::Remove(const std::shared_ptr<TmpClient>& tc)
{
	std::scoped_lock lock(mTmpClients);
	tmpClients.erase(tc);
}

void RoomHosting::DoAccept()
{
	acceptor.async_accept(
	[this](const std::error_code& ec, asio::ip::tcp::socket soc)
	{
		if(!acceptor.is_open())
			return;
		if(!ec)
		{
			Workaround::SetCloseOnExec(soc.native_handle());
			auto tc = std::make_shared<TmpClient>(std::move(soc));
			Add(tc);
			DoReadHeader(tc);
		}
		DoAccept();
	});
}

void RoomHosting::DoReadHeader(const std::shared_ptr<TmpClient>& tc)
{
	auto buffer = asio::buffer(tc->msg.Data(), YGOPro::CTOSMsg::HEADER_LENGTH);
	asio::async_read(tc->soc, buffer,
	[this, tc](const std::error_code& ec, std::size_t /*unused*/)
	{
		if(!ec && tc->msg.IsHeaderValid())
			DoReadBody(tc);
		else
			Remove(tc);
	});
}

void RoomHosting::DoReadBody(const std::shared_ptr<TmpClient>& tc)
{
	auto buffer = asio::buffer(tc->msg.Body(), tc->msg.GetLength());
	asio::async_read(tc->soc, buffer,
	[this, tc](const std::error_code& ec, std::size_t /*unused*/)
	{
		if(!ec && HandleMsg(tc))
			DoReadHeader(tc);
		else
			Remove(tc);
	});
}

bool RoomHosting::HandleMsg(const std::shared_ptr<TmpClient>& tc)
{
	using namespace YGOPro;
#define UTF16_BUFFER_TO_STR(a) \
	UTF16ToUTF8(BufferToUTF16(a, sizeof(decltype(a))))
	auto SendVersionError = [](asio::ip::tcp::socket& s)
	{
		static const auto m = STOCMsgFactory::MakeVersionError(SERVER_VERSION);
		asio::write(s, asio::buffer(m.Data(), m.Length()));
		return false;
	};
	auto& msg = tc->msg;
	switch(msg.GetType())
	{
	case CTOSMsg::MsgType::PLAYER_INFO:
	{
		auto p = msg.GetPlayerInfo();
		if(!p)
			return false;
		tc->name = UTF16_BUFFER_TO_STR(p->name);
		return true;
	}
	case CTOSMsg::MsgType::CREATE_GAME:
	{
		auto p = msg.GetCreateGame();
		if(!p || p->hostInfo.handshake != SERVER_HANDSHAKE ||
		   p->hostInfo.version != SERVER_VERSION)
			return SendVersionError(tc->soc);
		p->notes[199] = '\0'; // NOLINT: Guarantee null-terminated string
		Room::Instance::CreateInfo info =
		{
			lobby,
			ioCtx,
			coreProvider,
			replayManager,
			scriptProvider,
			dataProvider.GetDatabase(),
			p->hostInfo,
			LimitsFromFlags(info.hostInfo.extraRules),
			banlistProvider.GetBanlistByHash(p->hostInfo.banlistHash),
			UTF16_BUFFER_TO_STR(p->name),
			std::string(p->notes),
			UTF16_BUFFER_TO_STR(p->pass)
		};
		// Fix some of the options back into expected values in case of
		// exceptions.
		if(info.banlist == nullptr)
			info.hostInfo.banlistHash = 0;
		info.hostInfo.t0Count = std::clamp(info.hostInfo.t0Count, 1, 3);
		info.hostInfo.t1Count = std::clamp(info.hostInfo.t1Count, 1, 3);
		info.hostInfo.bestOf = std::max(info.hostInfo.bestOf, 1);
		// Make new room with the set parameters
		auto room = std::make_shared<Room::Instance>(std::move(info));
		room->RegisterToOwner();
		// Add the client to the newly created room
		auto client = std::make_shared<Room::Client>(
			*room,
			std::move(tc->soc),
			std::move(tc->name));
		client->RegisterToOwner();
		client->Start(std::move(room));
		return false;
	}
	case CTOSMsg::MsgType::JOIN_GAME:
	{
		auto p = msg.GetJoinGame();
		if(!p || p->version != SERVER_VERSION)
			return SendVersionError(tc->soc);
		auto room = lobby.GetRoomById(p->id);
		if(room && room->CheckPassword(UTF16_BUFFER_TO_STR(p->pass)))
		{
			auto client = std::make_shared<Room::Client>(
				*room,
				std::move(tc->soc),
				std::move(tc->name));
			client->RegisterToOwner();
			client->Start(std::move(room));
		}
		return false;
	}
	default: return false;
	}
#undef UTF16_BUFFER_TO_STR
}

} // namespace Ignis::Multirole::Endpoint
