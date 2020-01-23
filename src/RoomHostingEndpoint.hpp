#ifndef ROOMHOSTINGENDPOINT_HPP
#define ROOMHOSTINGENDPOINT_HPP
#include <asio.hpp>

namespace Ignis
{

class Lobby;

class RoomHostingEndpoint
{
public:
	RoomHostingEndpoint(asio::io_context& ioContext, unsigned short port, Lobby& lobby);
private:
	asio::ip::tcp::acceptor acceptor;
	Lobby& lobby;

	void DoAccept();
};

} // namespace Ignis

#endif // ROOMHOSTINGENDPOINT_HPP
