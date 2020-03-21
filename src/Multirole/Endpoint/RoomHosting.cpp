#include "RoomHosting.hpp"

#include <type_traits> // std::remove_extent

#include <asio/read.hpp>

#include "../Client.hpp"
#include "../Lobby.hpp"
#include "../Room.hpp"
#include "../YGOPro/CTOSMsg.hpp"
#include "../YGOPro/StringUtils.hpp"

namespace Ignis::Multirole::Endpoint
{

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

RoomHosting::RoomHosting(asio::io_context& ioCtx, unsigned short port,
                         CoreProvider& coreProvider,
                         BanlistProvider& banlistProvider, Lobby& lobby) :
	ioCtx(ioCtx),
	acceptor(ioCtx, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
	coreProvider(coreProvider),
	banlistProvider(banlistProvider),
	lobby(lobby)
{
	DoAccept();
}

void RoomHosting::Stop()
{
	acceptor.close();
	std::lock_guard<std::mutex> lock(mTmpClients);
	for(auto& c : tmpClients)
		c->soc.cancel();
}

// private

void RoomHosting::Add(const std::shared_ptr<TmpClient>& tc)
{
	std::lock_guard<std::mutex> lock(mTmpClients);
	tmpClients.insert(tc);
}

void RoomHosting::Remove(const std::shared_ptr<TmpClient>& tc)
{
	std::lock_guard<std::mutex> lock(mTmpClients);
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
#define UTF16_BUFFER_TO_STR(a) \
	YGOPro::UTF16ToUTF8(YGOPro::BufferToUTF16(a, sizeof(decltype(a))))
	auto& msg = tc->msg;
	switch(msg.GetType())
	{
	case YGOPro::CTOSMsg::MsgType::PLAYER_INFO:
	{
		auto p = msg.GetPlayerInfo();
		if(!p.first)
			return false;
		tc->name = UTF16_BUFFER_TO_STR(p.second.name);
		return true;
	}
	case YGOPro::CTOSMsg::MsgType::CREATE_GAME:
	{
		auto p = msg.GetCreateGame();
		if(!p.first)
			return false;
		// TODO: verify game settings
		// TODO: maybe check server handshake?
		p.second.notes[199] = '\0'; // NOLINT: Guarantee null-terminated string
		Room::Options options;
		options.info = p.second.info;
		options.name = UTF16_BUFFER_TO_STR(p.second.name);
		options.notes = std::string(p.second.notes);
		options.pass = UTF16_BUFFER_TO_STR(p.second.pass);
		if(options.info.banlistHash != 0)
		{
			options.banlist = banlistProvider.GetBanlistByHash(options.info.banlistHash);
			if(options.banlist == nullptr)
				return false;
		}
		options.corePkg = coreProvider.GetCorePkg();
		auto room = std::make_shared<Room>(lobby, ioCtx, std::move(options));
		room->RegisterToOwner();
		auto client = std::make_shared<Client>(*room, *room, room->Strand(), std::move(tc->soc), std::move(tc->name));
		client->RegisterToOwner();
		client->Start();
		return false;
	}
	case YGOPro::CTOSMsg::MsgType::JOIN_GAME:
	{
		auto p = msg.GetJoinGame();
		if(!p.first)
			return false;
		// TODO: maybe check game version?
		auto room = lobby.GetRoomById(p.second.id);
		if(room && room->CheckPassword(UTF16_BUFFER_TO_STR(p.second.pass)))
		{
			auto client = std::make_shared<Client>(*room, *room, room->Strand(), std::move(tc->soc), std::move(tc->name));
			client->RegisterToOwner();
			client->Start();
		}
		return false;
	}
	default: return false;
	}
#undef UTF16_BUFFER_TO_STR
}

} // namespace Ignis::Multirole::Endpoint
