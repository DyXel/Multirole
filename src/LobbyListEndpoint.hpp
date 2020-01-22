#ifndef LOBBYLISTENDPOINT_HPP
#define LOBBYLISTENDPOINT_HPP
#include <memory>
#include <asio.hpp>

namespace Ignis
{

class Lobby;

class LobbyListEndpoint final
{
public:
	LobbyListEndpoint(asio::io_context& ioContext, unsigned short port, Lobby& lobby);

	void Stop();
	void Terminate();
private:
	asio::ip::tcp::acceptor acceptor;
	Lobby& lobby;

	std::string ComposeMsg();
	void DoAccept();
	void DoSendRoomList(asio::ip::tcp::socket soc);
};

} // namespace Ignis

#endif // LOBBYLISTENDPOINT_HPP
