#include "RoomHostingEndpoint.hpp"

namespace Ignis
{

namespace Multirole {

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

void RoomHostingEndpoint::DoAccept()
{
	acceptor.async_accept(
	[this](const std::error_code& ec, asio::ip::tcp::socket soc)
	{
		if(!acceptor.is_open())
			return;
		if(!ec)
		{
// 			DoSendRoomList(std::move(soc));
		}
		DoAccept();
	});
}

} // namespace Multirole

} // namespace Ignis
