#ifndef ROOM_INSTANCE_HPP
#define ROOM_INSTANCE_HPP
#include <set>
#include <string>
#include <string_view>

#include <asio/io_context.hpp>
#include <asio/io_context_strand.hpp>

#include "Context.hpp"
#include "TimerAggregator.hpp"

namespace Ignis::Multirole
{

class IRoomManager;

namespace Room
{

class Instance final : public std::enable_shared_from_this<Instance>
{
public:
	// Data passed on the ctor.
	struct CreateInfo
	{
		IRoomManager& owner;
		asio::io_context& ioCtx;
		CoreProvider& coreProvider;
		ReplayManager& replayManager;
		ScriptProvider& scriptProvider;
		std::shared_ptr<CardDatabase> cdb;
		YGOPro::HostInfo hostInfo;
		YGOPro::DeckLimits limits;
		YGOPro::BanlistPtr banlist;
		std::string name;
		std::string notes;
		std::string pass;
	};

	// Properties are queried data about the room for listing.
	struct Properties
	{
		YGOPro::HostInfo hostInfo;
		std::string notes;
		bool passworded;
		bool started;
		uint32_t id;
		std::map<uint8_t, std::string> duelists;
	};

	// Ctor and registering.
	Instance(CreateInfo&& info);
	void RegisterToOwner();

	// Check if the room state is not Waiting.
	bool Started() const;

	// Check if the given string matches the set password,
	// always return true if the password is empty.
	bool CheckPassword(std::string_view str) const;

	// Query properties of the room.
	Properties GetProperties() const;

	// Tries to remove the room if its not started.
	void TryClose();

	void Add(std::shared_ptr<Client> client);
	void Remove(std::shared_ptr<Client> client);
	asio::io_context::strand& Strand();
	void Dispatch(const EventVariant& e);
private:
	IRoomManager& owner;
	asio::io_context::strand strand;
	TimerAggregator tagg;
	Context ctx;
	StateVariant state;
	const std::string name;
	const std::string notes;
	const std::string pass;
	uint32_t id;

	std::set<std::shared_ptr<Client>> clients;
	std::mutex mClients;
};

} // namespace Room

} // namespace Ignis::Multirole

#endif // ROOM_INSTANCE_HPP
