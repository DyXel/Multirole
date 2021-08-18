#include "TimerAggregator.hpp"

#include <boost/asio/bind_executor.hpp>

#include "Instance.hpp"

namespace Ignis::Multirole::Room
{

TimerAggregator::TimerAggregator(Instance& room) :
	room(room),
	strand(room.Strand()),
	timers({AsioTimer(strand.context()), AsioTimer(strand.context())})
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
	timers[team].async_wait(boost::asio::bind_executor(strand,
	[team, room = room.shared_from_this()](boost::system::error_code ec)
	{
		if(!ec)
			room->Dispatch(Event::TimerExpired{team});
	}));
}

AsioTimer::time_point TimerAggregator::Expiry(uint8_t team) const
{
	assert(team <= 1U);
	return timers[team].expiry();
}


} // Ignis::Multirole::Room
