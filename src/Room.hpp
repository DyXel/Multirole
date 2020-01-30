#ifndef ROOM_HPP
#define ROOM_HPP
#include <string>
#include <string_view>
#include <memory>
#include <set>
#include <mutex>
#include <map>

#include <asio/io_context.hpp>
#include <asio/io_context_strand.hpp>

#include "Client.hpp"
#include "IClientListener.hpp"
#include "IClientManager.hpp"
#include "MsgCommon.hpp"

namespace YGOPro
{

class STOCMsg;

} // namespace YGOPro

namespace Ignis
{

namespace Multirole {

class IRoomManager;

class Room final : public IClientListener, public IClientManager, public std::enable_shared_from_this<Room>
{
public:
	enum StateEnum
	{
		WAITING, // Choosing decks, getting ready, etc.
		RPS, // Deciding which team/player goes first.
		DUELING, // Running duel Process and getting responses.
		SIDE_DECKING, // Self explanatory.
// 		REMATCHING, // Asking duelists for a rematch.
	};

	// Options are data that the room needs to function properly
	struct Options
	{
		YGOPro::HostInfo info;
		std::string name;
		std::string notes;
		std::string pass;
// 		std::shared_ptr<Banlist> banlist;
// 		std::shared_ptr<ICoreWrapper> core; // maybe has database and script reader with it?
		uint32_t id;
	};

	// Properties are queried data about the room for listing
	struct Properties
	{
		YGOPro::HostInfo info;
		std::string notes;
		bool passworded;
		uint32_t id;
		StateEnum state;
		std::map<int, std::string> duelists;
	};

	Room(IRoomManager& owner, asio::io_context& ioCtx, Options options);
	StateEnum State() const;
	bool CheckPassword(std::string_view str) const;
	void RegisterToOwner();
	Properties GetProperties();
	asio::io_context::strand& Strand();
	void TryClose();
private:
	IRoomManager& owner;
	asio::io_context::strand strand;
	Options options;
	StateEnum state;

	std::set<std::shared_ptr<Client>> clients;
	Client* host;
	std::map<Client::PositionType, Client*> duelists;
	std::mutex mClients;
	std::mutex mDuelists;

	void OnJoin(Client& client) override;
	void OnConnectionLost(Client& client) override;
	void OnChat(Client& client, std::string_view str) override;

	void Add(std::shared_ptr<Client> client) override;
	void Remove(std::shared_ptr<Client> client) override;

	void JoinToWaiting(Client& client);
	void JoinToDuel(Client& client);

	void PostUnregisterFromOwner();

	void SendToAll(const YGOPro::STOCMsg& msg);
};

} // namespace Multirole

} // namespace Ignis

#endif // ROOM_HPP
