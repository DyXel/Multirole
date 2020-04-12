#ifndef ROOM_CLIENT_HPP
#define ROOM_CLIENT_HPP
#include <utility>
#include <queue>
#include <mutex>

#include <asio/io_context_strand.hpp>
#include <asio/ip/tcp.hpp>

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

	Client(Instance& room, asio::ip::tcp::socket&& socket, std::string&& name);
	void RegisterToOwner();
	void Start(std::shared_ptr<Instance>&& ptr);

	// Getters
	std::string Name() const;
	PosType Position() const;
	bool Ready() const;
	const YGOPro::Deck* OriginalDeck() const;
	// Returns current deck or original (NOTE: this might still null)
	const YGOPro::Deck* CurrentDeck() const;

	// Setters
	void SetPosition(const PosType& p);
	void SetReady(bool r);
	void SetOriginalDeck(std::unique_ptr<YGOPro::Deck>&& newDeck);
	void SetCurrentDeck(std::unique_ptr<YGOPro::Deck>&& newDeck);

	// Adds a message to the queue that is written to the client socket
	void Send(const YGOPro::STOCMsg& msg);

	// Immediately disconnects from the room, cancelling all outstanding write
	// operations.
	void Disconnect();

	// Tries to disconnect immediately if there are no messages in the queue,
	// sets a flag if there are messages in the queue to disconnect
	// upon finishing writes.
	void DeferredDisconnect();
private:
	Instance& room;
	asio::io_context::strand& strand;
	asio::ip::tcp::socket socket;
	bool disconnecting;
	std::string name;
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

	// Handles received CTOS message
	void HandleMsg();
};

} // namespace Ignis::Multirole::Room

#endif // ROOM_CLIENT_HPP
