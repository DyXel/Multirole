#ifndef LOBBYLISTING_HPP
#define LOBBYLISTING_HPP
#include <memory>
#include <mutex>

#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>

namespace Ignis::Multirole
{

class Lobby;

namespace Endpoint
{

class LobbyListing final
{
public:
	LobbyListing(asio::io_context& ioCtx, unsigned short port, Lobby& lobby);
	~LobbyListing();

	void Stop();
private:
	class Connection final : public std::enable_shared_from_this<Connection>
	{
	public:
		Connection(asio::ip::tcp::socket socket, std::shared_ptr<std::string> data);
		void DoRead();
	private:
		asio::ip::tcp::socket socket;
		std::shared_ptr<std::string> outgoing;
		std::array<char, 256> incoming;
		bool writeCalled;

		void DoWrite();
	};

	asio::ip::tcp::acceptor acceptor;
	asio::steady_timer serializeTimer;
	Lobby& lobby;
	std::shared_ptr<std::string> serialized;
	std::mutex mSerialized;

	void DoAccept();
	void DoSerialize();
};

} // namespace Endpoint

} // namespace Ignis::Multirole

#endif // LOBBYLISTING_HPP
