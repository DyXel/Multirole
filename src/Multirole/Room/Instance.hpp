#ifndef ROOM_INSTANCE_HPP
#define ROOM_INSTANCE_HPP
#include <set>
#include <string>
#include <string_view>

#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>

#include "Context.hpp"
#include "TimerAggregator.hpp"
#include "../RNG/Xoshiro256.hpp"

namespace Ignis::Multirole::Room
{

class Instance final : public std::enable_shared_from_this<Instance>
{
public:
	// Data passed on the ctor.
	struct CreateInfo
	{
		boost::asio::io_context& ioCtx;
		std::string notes;
		std::string pass;
		Service& svc;
		uint32_t id;
		RNG::Xoshiro256StarStar::StateType seed;
		YGOPro::BanlistPtr banlist;
		YGOPro::HostInfo hostInfo;
		YGOPro::DeckLimits limits;
	};

	// Ctor and registering.
	Instance(CreateInfo& info) noexcept;

	// Get whether or not the room is private (has password set).
	bool IsPrivate() const noexcept;

	// Check if the room state is not Waiting.
	bool Started() const noexcept;

	// Get the notes of the room.
	const std::string& Notes() const noexcept;

	// Get the game options of the room.
	const YGOPro::HostInfo& HostInfo() const noexcept;

	// Get each duelist index (encoded position) along with their name.
	DuelistsMap DuelistNames() const noexcept;

	// Check if the given string matches the set password,
	// always return true if the password is empty.
	bool CheckPassword(std::string_view str) const noexcept;

	// Check whether or not the IP was kicked before from this room.
	bool CheckKicked(std::string_view ip) const noexcept;

	// Tries to remove the room if its not started.
	// Returns true if room was signaled, false otherwise.
	bool TryClose() noexcept;

	// Adds an IP to the kicked list, checked with CheckKicked.
	void AddKicked(std::string_view ip) noexcept;

	boost::asio::io_context::strand& Strand() noexcept;
	void Dispatch(const EventVariant& e) noexcept;
private:
	boost::asio::io_context::strand strand;
	TimerAggregator tagg;
	const std::string notes;
	const std::string pass;
	Context ctx;

	StateVariant state;
	mutable std::shared_mutex mState;

	std::set<std::string> kicked;
	mutable std::mutex mKicked;
};

} // namespace Ignis::Multirole::Room

#endif // ROOM_INSTANCE_HPP
