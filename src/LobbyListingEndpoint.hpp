#ifndef LOBBYLISTINGENDPOINT_HPP
#define LOBBYLISTINGENDPOINT_HPP
#include <memory>
#include <asio.hpp>

namespace Ignis
{

namespace Multirole {

class Lobby;

class LobbyListingEndpoint final
{
public:
	LobbyListingEndpoint(asio::io_context& ioContext, unsigned short port, Lobby& lobby);
	void Stop();
private:
	asio::ip::tcp::acceptor acceptor;
	Lobby& lobby;

	std::string ComposeMsg();
	void DoAccept();
	void DoSendRoomList(asio::ip::tcp::socket soc);
};

} // namespace Multirole

} // namespace Ignis

#endif // LOBBYLISTINGENDPOINT_HPP
