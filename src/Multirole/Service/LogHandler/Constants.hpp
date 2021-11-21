#ifndef MULTIROLE_SERVICE_LOGHANDLER_CONSTANTS_HPP
#define MULTIROLE_SERVICE_LOGHANDLER_CONSTANTS_HPP
#include <cstdint>

namespace Ignis::Multirole
{

enum class ServiceType
{
	GIT_REPO,
	MULTIROLE,
	BANLIST_PROVIDER,
	CORE_PROVIDER,
	DATA_PROVIDER,
	LOG_HANDLER,
	REPLAY_MANAGER,
	SCRIPT_PROVIDER,
	SERVICE_TYPE_COUNT
};

#undef ERROR // NOTE: MSVC seems to define this but we don't care about it.

enum class Level
{
	INFO,
	WARN,
	ERROR,
	LEVEL_COUNT
};

enum class ErrorCategory
{
	CORE,
	OFFICIAL,
	SPEED,
	RUSH,
	UNOFFICIAL,
	ERROR_CATEGORY_COUNT
};

} // namespace Ignis::Multirole

#endif // MULTIROLE_SERVICE_LOGHANDLER_CONSTANTS_HPP
