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
		asio::io_context& ioCtx;
		IRoomManager& owner;
		Service& svc;
		std::string name;
		std::string notes;
		std::string pass;
		YGOPro::BanlistPtr banlist;
		YGOPro::HostInfo hostInfo;
		YGOPro::DeckLimits limits;
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

	// Check whether or not the IP was kicked before from this room.
	bool CheckKicked(const asio::ip::address& addr) const;

	// Query properties of the room.
	Properties GetProperties() const;

	// Tries to remove the room if its not started.
	void TryClose();

	// Adds an IP to the kicked list, checked with CheckKicked.
	void AddKicked(const asio::ip::address& addr);

	void Add(std::shared_ptr<Client> client);
	void Remove(std::shared_ptr<Client> client);
	asio::io_context::strand& Strand();
	void Dispatch(const EventVariant& e);
private:
	IRoomManager& owner;
	asio::io_context::strand strand;
	TimerAggregator tagg;
	const std::string name;
	const std::string notes;
	const std::string pass;
	uint32_t id;
	Context ctx;
	StateVariant state;

	std::set<std::shared_ptr<Client>> clients;
	std::mutex mClients;

	std::set<asio::ip::address> kicked;
	mutable std::mutex mKicked;
};

} // namespace Room

} // namespace Ignis::Multirole

#endif // ROOM_INSTANCE_HPP
