#ifndef ROOM_CLIENT_HPP
#define ROOM_CLIENT_HPP
#include <utility>
#include <queue>
#include <mutex>

#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "../YGOPro/CTOSMsg.hpp"
#include "../YGOPro/Deck.hpp"
#include "../YGOPro/STOCMsg.hpp"

namespace Ignis::Multirole
{

class Lobby;

namespace Room
{

class Instance;

class Client final : public std::enable_shared_from_this<Client>
{
public:
	using PosType = std::pair<uint8_t, uint8_t>;
	static constexpr PosType POSITION_SPECTATOR = {UINT8_MAX, UINT8_MAX};

	Client(Lobby& lobby, std::shared_ptr<Instance> r, boost::asio::ip::tcp::socket socket, std::string ip, std::string name) noexcept;
	~Client() noexcept;
	void Start() noexcept;

	// Getters
	const std::string& Ip() const noexcept;
	const std::string& Name() const noexcept;
	PosType Position() const noexcept;
	bool Ready() const noexcept;
	const YGOPro::Deck* OriginalDeck() const noexcept;
	// Returns current deck or original (NOTE: this might still be nullptr)
	const YGOPro::Deck* CurrentDeck() const noexcept;

	// Set this client as kicked from the room its in, preventing its IP
	// from joining in the future.
	void MarkKicked() const noexcept;

	// Setters
	void SetPosition(const PosType& p) noexcept;
	void SetReady(bool r) noexcept;
	void SetOriginalDeck(std::unique_ptr<YGOPro::Deck>&& newDeck) noexcept;
	void SetCurrentDeck(std::unique_ptr<YGOPro::Deck>&& newDeck) noexcept;

	// Adds a message to the queue that is written to the client socket
	void Send(const YGOPro::STOCMsg& msg) noexcept;

	// Tries to disconnect immediately if there are no messages in the queue,
	// sets a flag if there are messages in the queue to disconnect
	// upon finishing writes.
	void Disconnect() noexcept;
private:
	Lobby& lobby;
	std::shared_ptr<Instance> room;
	boost::asio::io_context::strand& strand;
	boost::asio::ip::tcp::socket socket;
	const std::string ip;
	const std::string name;
	bool disconnecting;
	PosType position;
	bool ready;
	std::unique_ptr<YGOPro::Deck> originalDeck;
	std::unique_ptr<YGOPro::Deck> currentDeck;

	// Message data
	YGOPro::CTOSMsg incoming;
	std::queue<YGOPro::STOCMsg> outgoing;
	std::mutex mOutgoing;

	// Asynchronous calls
	void DoReadHeader() noexcept;
	void DoReadBody() noexcept;
	void DoWrite() noexcept;

	// Shuts down socket immediately, disallowing any read or writes,
	// doing that starts the graceful connection closure.
	void Shutdown() noexcept;

	// Handles received CTOS message
	void HandleMsg() noexcept;
};

} // namespace Room

} // namespace Ignis::Multirole

#endif // ROOM_CLIENT_HPP
