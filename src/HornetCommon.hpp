#ifndef HORNETCOMMON_HPP
#define HORNETCOMMON_HPP
#include <array>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

namespace ipc = boost::interprocess;

namespace Ignis::Hornet
{

using LockType = ipc::scoped_lock<ipc::interprocess_mutex>;

enum class Action : uint8_t
{
	// Triggerable callbacks: doesn't apply
	NO_WORK = 0U,

	// Triggerable callbacks: doesn't apply
	EXIT,

	// Triggerable callbacks: none
	OCG_GET_VERSION,

	// Triggerable callbacks: LoadScript
	OCG_CREATE_DUEL,

	// Triggerable callbacks: none
	OCG_DESTROY_DUEL,

	// Triggerable callbacks: ?
	OCG_DUEL_NEW_CARD,

	// Triggerable callbacks: none
	OCG_START_DUEL,

	// Triggerable callbacks: ?
	OCG_DUEL_PROCESS,

	// Triggerable callbacks: none
	OCG_DUEL_GET_MESSAGE,

	// Triggerable callbacks: none
	OCG_DUEL_SET_RESPONSE,

	// Triggerable callbacks: ?
	OCG_LOAD_SCRIPT,

	// Triggerable callbacks: none
	OCG_DUEL_QUERY_COUNT,

	// Triggerable callbacks: none
	OCG_DUEL_QUERY,

	// Triggerable callbacks: none
	OCG_DUEL_QUERY_LOCATION,

	// Triggerable callbacks: none
	OCG_DUEL_QUERY_FIELD,

	// Triggerable callbacks: doesn't apply
	CB_DATA_READER,

	// Triggerable callbacks: doesn't apply
	CB_SCRIPT_READER,

	// Triggerable callbacks: doesn't apply
	CB_LOG_HANDLER,

	// Triggerable callbacks: doesn't apply
	CB_DATA_READER_DONE,

	// Triggerable callbacks: doesn't apply
	CB_DONE,
};

struct SharedSegment
{
	ipc::interprocess_mutex mtx;
	ipc::interprocess_condition cv;
	Action act{Action::NO_WORK};
	std::array<uint8_t, std::numeric_limits<uint16_t>::max()> bytes{};
};

} // namespace Ignis::Hornet

#endif // HORNETCOMMON_HPP
