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

namespace Room
{

class Instance final : public std::enable_shared_from_this<Instance>
{
public:
	// Data passed on the ctor.
	struct CreateInfo
	{
		asio::io_context& ioCtx;
		std::string notes;
		std::string pass;
		Service& svc;
		uint32_t id;
		uint32_t seed;
		YGOPro::BanlistPtr banlist;
		YGOPro::HostInfo hostInfo;
		YGOPro::DeckLimits limits;
	};

	// Ctor and registering.
	Instance(CreateInfo& info);

	// Get whether or not the room is private (has password set).
	bool IsPrivate() const;

	// Check if the room state is not Waiting.
	bool Started() const;

	// Get the notes of the room.
	const std::string& Notes() const;

	// Get the game options of the room.
	const YGOPro::HostInfo& HostInfo() const;

	// Get each duelist index along with their name.
	std::map<uint8_t, std::string> DuelistNames() const;

	// Check if the given string matches the set password,
	// always return true if the password is empty.
	bool CheckPassword(std::string_view str) const;

	// Check whether or not the IP was kicked before from this room.
	bool CheckKicked(const asio::ip::address& addr) const;

	// Tries to remove the room if its not started.
	void TryClose();

	// Adds an IP to the kicked list, checked with CheckKicked.
	void AddKicked(const asio::ip::address& addr);

	void Add(const std::shared_ptr<Client>& client);
	void Remove(const std::shared_ptr<Client>& client);
	asio::io_context::strand& Strand();
	void Dispatch(const EventVariant& e);
private:
	asio::io_context::strand strand;
	TimerAggregator tagg;
	const std::string notes;
	const std::string pass;
	const bool isPrivate;
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
