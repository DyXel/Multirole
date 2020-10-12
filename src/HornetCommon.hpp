#ifndef HORNETCOMMON_HPP
#define HORNETCOMMON_HPP
#include <array>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

namespace ipc = boost::interprocess;

namespace Ignis::Hornet
{

enum class Action : uint8_t
{
	// Triggerable callbacks: doesn't apply
	EXIT,

	// Triggerable callbacks: doesn't apply
	GOODBYE,

	// Triggerable callbacks: none
	OCG_GET_VERSION,

	// Triggerable callbacks: none
	OCG_CREATE_DUEL,

	// Triggerable callbacks: ?
	OCG_DUEL_NEW_CARD,

	// Triggerable callbacks: ?
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
};

struct SharedSegment
{
	ipc::interprocess_mutex mtx;
	ipc::interprocess_condition cv;
	Action act;
	std::array<uint8_t, std::numeric_limits<uint16_t>::max()> bytes;
};

} // namespace Ignis::Hornet

#endif // HORNETCOMMON_HPP
