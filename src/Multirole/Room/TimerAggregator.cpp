#include "TimerAggregator.hpp"

#include "Instance.hpp"

namespace Ignis::Multirole::Room
{

inline std::array<AsioTimer, 2U> MakeTimers(asio::io_context::strand& strand)
{
	return {AsioTimer{strand}, AsioTimer{strand}};
}

TimerAggregator::TimerAggregator(Instance& room) :
	room(room),
	timers(MakeTimers(room.Strand()))
{}

void TimerAggregator::Cancel(uint8_t team)
{
	assert(team <= 1U);
	timers[team].cancel();
}

void TimerAggregator::ExpiresAfter(uint8_t team, const AsioTimer::duration& expiryTime)
{
	assert(team <= 1U);
	timers[team].expires_after(expiryTime);
	timers[team].async_wait([this, team](const asio::error_code& ec)
	{
		if(!ec)
			room.Dispatch(Event::TimerExpired{team});
	});
}

AsioTimer::time_point TimerAggregator::Expiry(uint8_t team) const
{
	assert(team <= 1U);
	return timers[team].expiry();
}


} // Ignis::Multirole::Room
