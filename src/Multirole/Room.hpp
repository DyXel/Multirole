#ifndef ROOM_HPP
#define ROOM_HPP
#include <string>
#include <string_view>
#include <memory>
#include <set>
#include <mutex>
#include <map>
#include <random> // std::mt19937

#include <asio/io_context.hpp>
#include <asio/io_context_strand.hpp>

#include "CoreProvider.hpp"
#include "Client.hpp"
#include "IClientListener.hpp"
#include "IClientManager.hpp"
#include "STOCMsgFactory.hpp"

namespace YGOPro
{

class Banlist;
class STOCMsg;

} // namespace YGOPro

namespace Ignis::Multirole
{

namespace Core
{

class IHighLevelWrapper;

} // namespace Core

class IRoomManager;

class Room final :
	public IClientListener,
	public IClientManager,
	public STOCMsgFactory,
	public std::enable_shared_from_this<Room>
{
public:
	enum StateEnum
	{
		STATE_WAITING, // Choosing decks, getting ready, etc.
		STATE_RPS, // Deciding which team/player goes first.
		STATE_DUELING, // Running duel Process and getting responses.
		STATE_SIDE_DECKING, // Self explanatory.
		STATE_REMATCHING, // Asking duelists for a rematch.
	};

	// Options are data that the room needs to function properly
	struct Options
	{
		YGOPro::HostInfo info;
		YGOPro::DeckLimits limits;
		std::string name;
		std::string notes;
		std::string pass;
		const YGOPro::Banlist* banlist;
		CoreProvider::CorePkg cpkg;
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

	// Ctor and registering
	Room(IRoomManager& owner, asio::io_context& ioCtx, Options&& options);
	void RegisterToOwner();

	// Getter
	StateEnum State() const;

	// Check if the string matches options.pass,
	// always return true if options.pass is empty.
	bool CheckPassword(std::string_view str) const;

	// Non-const Getters
	Properties GetProperties();
	asio::io_context::strand& Strand();

	// Tries to remove the room if its waiting
	void TryClose();
private:
	IRoomManager& owner;
	asio::io_context::strand strand;

	std::set<std::shared_ptr<Client>> clients;
	Client* host;
	std::map<Client::PosType, Client*> duelists;
	std::mutex mClients;
	std::mutex mDuelists;

	// Options this room was created with
	Options options;

	// States data
	StateEnum state;
	struct
	{
		Client* goingFirstSelector;
		struct
		{
			std::array<uint8_t, 2> c; // Choices
		}rps;
		struct
		{
// 			bool it0gf; // Is Team 0 Going First?
			std::unique_ptr<std::mt19937> rng; // Random number generator
			OCG_Duel ptr; // Pointer to duel, used by the core
		}duel;
	}states;

	// IClientListener overrides
	void OnJoin(Client& client) override;
	void OnConnectionLost(Client& client) override;
	void OnChat(Client& client, std::string_view str) override;
	void OnToDuelist(Client& client) override;
	void OnToObserver(Client& client) override;
	void OnUpdateDeck(Client& client, const std::vector<uint32_t>& main,
	                  const std::vector<uint32_t>& side) override;
	void OnReady(Client& client, bool value) override;
	void OnTryKick(Client& client, uint8_t pos) override;
	void OnTryStart(Client& client) override;
	void OnRPSChoice(Client& client, uint8_t value) override;
	void OnTurnChoice(Client& client, bool goingFirst) override;

	// IClientManager overrides
	void Add(std::shared_ptr<Client> client) override;
	void Remove(std::shared_ptr<Client> client) override;

	// Utilities to send a message to multiple clients
	void SendToAll(const YGOPro::STOCMsg& msg);
	void SendToTeam(uint8_t team, const YGOPro::STOCMsg& msg);

	// Join stuff
	bool TryEmplaceDuelist(Client& client, Client::PosType hint = {});
	void JoinToWaiting(Client& client);
	void JoinToDuel(Client& client);

	// Deck stuff
	std::unique_ptr<YGOPro::Deck> LoadDeck(const std::vector<uint32_t>& main,
	                                       const std::vector<uint32_t>& side) const;
	std::unique_ptr<YGOPro::STOCMsg> CheckDeck(const YGOPro::Deck& deck) const;

	void SendRPS(); // Post: STATE_RPS
	void StartDuel(bool isTeam0GoingFirst); // Post: STATE_DUELING
	void FinishDuel(); // Post: STATE_SIDE_DECKING or STATE_REMATCHING
};

} // namespace Ignis::Multirole

#endif // ROOM_HPP
