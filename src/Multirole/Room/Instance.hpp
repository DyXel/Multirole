#ifndef ROOM_INSTANCE_HPP
#define ROOM_INSTANCE_HPP
#include <set>
#include <string>
#include <string_view>

#include <asio/io_context.hpp>
#include <asio/io_context_strand.hpp>

#include "Context.hpp"

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
		YGOPro::HostInfo hostInfo;
		YGOPro::DeckLimits limits;
		CoreProvider::CorePkg cpkg;
		const YGOPro::Banlist* banlist;
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
	Instance(CreateInfo&& info, IRoomManager& owner, asio::io_context& ioCtx);
	void RegisterToOwner();

	// Check if the room state is not Waiting
	bool Started() const;

	// Check if the given string matches the set password,
	// always return true if the password is empty.
	bool CheckPassword(std::string_view str) const;

	// Query properties of the room.
	Properties GetProperties();

	// Tries to remove the room if its not started.
	void TryClose();

	// IClientManager overrides
	void Add(const std::shared_ptr<Client>& client);
	void Remove(const std::shared_ptr<Client>& client);

	// IClientListener overrides
	asio::io_context::strand& Strand();
	void Dispatch(EventVariant e);
private:
	IRoomManager& owner;
	asio::io_context::strand strand;
	Context ctx;
	StateVariant state;
	std::string name;
	std::string notes;
	std::string pass;
	uint32_t id{};

	std::set<std::shared_ptr<Client>> clients;
	std::mutex mClients;
};

} // namespace Room

} // namespace Ignis::Multirole

#endif // ROOM_INSTANCE_HPP
