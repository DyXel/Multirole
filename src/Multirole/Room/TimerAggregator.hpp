#ifndef ROOM_TIMER_AGGREGATOR_HPP
#define ROOM_TIMER_AGGREGATOR_HPP
#include <array>

#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/system_timer.hpp>

namespace Ignis::Multirole::Room
{

class Instance;

using AsioTimer = boost::asio::system_timer;

class TimerAggregator final
{
public:
	TimerAggregator(Instance& room);

	// Cancels one timer's asynchronous operations that were set by
	// calling ExpireAfter.
	void Cancel(uint8_t team);

	// Sets one timer's expiry time relative to now, also sets
	// asynchronous operation to dispatch time-out event to room instance.
	void ExpiresAfter(uint8_t team, const AsioTimer::duration& expiryTime);

	// Gets one timer's expiry time as an absolute time.
	AsioTimer::time_point Expiry(uint8_t team) const;
private:
	Instance& room;
	boost::asio::io_context::strand& strand;
	std::array<AsioTimer, 2U> timers;
};

} // Ignis::Multirole::Room

#endif // ROOM_TIMER_AGGREGATOR_HPP
