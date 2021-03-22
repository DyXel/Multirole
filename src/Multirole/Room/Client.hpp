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

namespace Ignis::Multirole::Room
{

class Instance;

class Client final : public std::enable_shared_from_this<Client>
{
public:
	using PosType = std::pair<uint8_t, uint8_t>;
	static constexpr PosType POSITION_SPECTATOR = {UINT8_MAX, UINT8_MAX};

	Client(std::shared_ptr<Instance> r, boost::asio::ip::tcp::socket socket, std::string ip, std::string name);
	void Start();

	// Getters
	const std::string& Ip() const;
	const std::string& Name() const;
	PosType Position() const;
	bool Ready() const;
	const YGOPro::Deck* OriginalDeck() const;
	// Returns current deck or original (NOTE: this might still be nullptr)
	const YGOPro::Deck* CurrentDeck() const;

	// Set this client as kicked from the room its in, preventing its IP
	// from joining in the future.
	void MarkKicked() const;

	// Setters
	void SetPosition(const PosType& p);
	void SetReady(bool r);
	void SetOriginalDeck(std::unique_ptr<YGOPro::Deck>&& newDeck);
	void SetCurrentDeck(std::unique_ptr<YGOPro::Deck>&& newDeck);

	// Adds a message to the queue that is written to the client socket
	void Send(const YGOPro::STOCMsg& msg);

	// Tries to disconnect immediately if there are no messages in the queue,
	// sets a flag if there are messages in the queue to disconnect
	// upon finishing writes.
	void Disconnect();
private:
	std::shared_ptr<Instance> room;
	boost::asio::io_context::strand& strand;
	boost::asio::ip::tcp::socket socket;
	const std::string ip;
	const std::string name;
	bool connectionLost;
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
	void DoReadHeader();
	void DoReadBody();
	void DoWrite();

	// Shuts down socket immediately, disallowing any read or writes,
	// doing that starts the graceful connection closure.
	void Shutdown();

	// Handles received CTOS message
	void HandleMsg();
};

} // namespace Ignis::Multirole::Room

#endif // ROOM_CLIENT_HPP
