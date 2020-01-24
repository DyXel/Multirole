#include "RoomHostingEndpoint.hpp"

#include <fmt/printf.h>

#include "CTOSMsg.hpp"
#include "StringUtils.hpp"

namespace Ignis
{

namespace Multirole {

struct TmpClient
{
	asio::ip::tcp::socket soc;
	YGOPro::CTOSMsg msg;
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
}

// private

void RoomHostingEndpoint::Add(std::shared_ptr<TmpClient> tc)
{
	std::lock_guard<std::mutex> lock(m);
	tmpClients.insert(tc);
}

void RoomHostingEndpoint::Remove(std::shared_ptr<TmpClient> tc)
{
	std::lock_guard<std::mutex> lock(m);
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
			DoReadHeader(std::make_shared<TmpClient>(std::move(soc)));
		DoAccept();
	});
}

void RoomHostingEndpoint::DoReadHeader(std::shared_ptr<TmpClient> tc)
{
	auto buffer = asio::buffer(tc->msg.Data(), YGOPro::CTOSMsg::HEADER_LENGTH);
	asio::async_read(tc->soc, buffer,
	[this, tc](const std::error_code& ec, std::size_t)
	{
		if (!ec && tc->msg.IsHeaderValid())
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
		if (!ec && HandleMsg(tc))
			DoReadHeader(tc);
		else
			Remove(tc);
	});
}

bool RoomHostingEndpoint::HandleMsg(std::shared_ptr<TmpClient> tc)
{
	auto& msg = tc->msg;
	const auto msgLength = msg.GetLength();
	switch(msg.GetType())
	{
	case YGOPro::CTOSMsg::MsgType::PLAYER_INFO:
	{
		using namespace StringUtils;
		fmt::print("Client '{}' starts a new connection\n", UTF16ToUTF8(BufferToUTF16(msg.Body(), msgLength)));
		return true;
	}
	default: return false;
	}
}

} // namespace Multirole

} // namespace Ignis
