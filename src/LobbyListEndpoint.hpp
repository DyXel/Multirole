#ifndef LOBBYLISTENDPOINT_HPP
#define LOBBYLISTENDPOINT_HPP
#include <memory>
#include <asio.hpp>

namespace Ignis
{

class Lobby;

class LobbyListEndpoint
{
public:
	LobbyListEndpoint(asio::io_context& ioContext, short port, std::shared_ptr<Lobby> lobby);

	void Terminate();
private:
	asio::ip::tcp::acceptor acceptor;
	std::shared_ptr<Lobby> lobby;

	std::string ComposeMsg();
	void DoAccept();
	void DoSendRoomList(asio::ip::tcp::socket soc);
};

} // namespace Ignis

#endif // LOBBYLISTENDPOINT_HPP
