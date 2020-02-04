#ifndef LOBBYLISTING_HPP
#define LOBBYLISTING_HPP
#include <memory>
#include <mutex>

#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>

namespace Ignis
{

namespace Multirole
{

class Lobby;

namespace Endpoint
{

class LobbyListing final
{
public:
	LobbyListing(asio::io_context& ioCtx, unsigned short port, Lobby& lobby);
	void Stop();
private:
	asio::ip::tcp::acceptor acceptor;
	asio::steady_timer serializeTimer;
	Lobby& lobby;
	std::string serialized;
	std::mutex mSerialized;

	void DoAccept();
	void DoSerialize();
	void DoSendRoomList(asio::ip::tcp::socket soc);
};

} // namespace Endpoint

} // namespace Multirole

} // namespace Ignis

#endif // LOBBYLISTING_HPP
