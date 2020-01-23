#ifndef ROOMHOSTINGENDPOINT_HPP
#define ROOMHOSTINGENDPOINT_HPP
#include <asio.hpp>

namespace Ignis
{

namespace Multirole {

class Lobby;

class RoomHostingEndpoint
{
public:
	RoomHostingEndpoint(asio::io_context& ioContext, unsigned short port, Lobby& lobby);
	void Stop();
private:
	asio::ip::tcp::acceptor acceptor;
	Lobby& lobby;

	void DoAccept();
};

} // namespace Ignis

} // namespace Multirole

#endif // ROOMHOSTINGENDPOINT_HPP
