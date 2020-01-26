#include "RoomHostingEndpoint.hpp"

#include <type_traits> // std::remove_extent

#include "Client.hpp"
#include "Lobby.hpp"
#include "Room.hpp"
#include "CTOSMsg.hpp"
#include "StringUtils.hpp"

namespace Ignis
{

namespace Multirole {

// Holds information about the client before a proper connection to a Room
// has been established.
struct TmpClient
{
	asio::ip::tcp::socket soc;
	YGOPro::CTOSMsg msg;
	std::string name;

	TmpClient(asio::ip::tcp::socket soc) : soc(std::move(soc))
	{}
};

// public

RoomHostingEndpoint::RoomHostingEndpoint(
	asio::io_context& ioContext, unsigned short port, Lobby& lobby) :
	acceptor(ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
	lobby(lobby)
{
	DoAccept();
}

void RoomHostingEndpoint::Stop()
{
	acceptor.close();
	std::lock_guard<std::mutex> lock(mTmpClients);
	for(auto& c : tmpClients)
		c->soc.cancel();
}

// private

void RoomHostingEndpoint::Add(std::shared_ptr<TmpClient> tc)
{
	std::lock_guard<std::mutex> lock(mTmpClients);
	tmpClients.insert(tc);
}

void RoomHostingEndpoint::Remove(std::shared_ptr<TmpClient> tc)
{
	std::lock_guard<std::mutex> lock(mTmpClients);
	tmpClients.erase(tc);
}

void RoomHostingEndpoint::DoAccept()
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
			DoReadHeader(std::move(tc));
		}
		DoAccept();
	});
}

void RoomHostingEndpoint::DoReadHeader(std::shared_ptr<TmpClient> tc)
{
	auto buffer = asio::buffer(tc->msg.Data(), YGOPro::CTOSMsg::HEADER_LENGTH);
	asio::async_read(tc->soc, buffer,
	[this, tc](const std::error_code& ec, std::size_t)
	{
		if(!ec && tc->msg.IsHeaderValid())
			DoReadBody(tc);
		else
			Remove(tc);
	});
}

void RoomHostingEndpoint::DoReadBody(std::shared_ptr<TmpClient> tc)
{
	auto buffer = asio::buffer(tc->msg.Body(), tc->msg.GetLength());
	asio::async_read(tc->soc, buffer,
	[this, tc](const std::error_code& ec, std::size_t)
	{
		if(!ec && HandleMsg(tc))
			DoReadHeader(tc);
		else
			Remove(tc);
	});
}

bool RoomHostingEndpoint::HandleMsg(std::shared_ptr<TmpClient> tc)
{
#define UTF16_BUFFER_TO_STR(a) \
	StringUtils::UTF16ToUTF8(StringUtils::BufferToUTF16(a, \
	sizeof(decltype(a)) / sizeof(std::remove_extent<decltype(a)>::type)))
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
		p.second.notes[200] = '\0'; // Guarantee null-terminated string
		Room::Options opts;
		opts.name = UTF16_BUFFER_TO_STR(p.second.name);
		opts.pass = UTF16_BUFFER_TO_STR(p.second.pass);
		opts.notes = std::string(p.second.notes);
		auto room = std::make_shared<Room>(lobby, std::move(opts));
		std::make_shared<Client>(*room, std::move(tc->prop), std::move(tc->soc));
		std::make_shared<Client>(*room, std::move(tc->name), std::move(tc->soc));
		return false;
	}
	default: return false;
	}
#undef UTF16_BUFFER_TO_STR
}

} // namespace Multirole

} // namespace Ignis
