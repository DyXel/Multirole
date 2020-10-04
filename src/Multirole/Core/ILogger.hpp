#ifndef ILOGGER_HPP
#define ILOGGER_HPP
#include <string_view>

namespace Ignis::Multirole::Core
{

class ILogger
{
public:
	enum class LogType
	{
		LOG_TYPE_ERROR,
		LOG_TYPE_FROM_SCRIPT,
		LOG_TYPE_FOR_DEBUG,
	};

	virtual void Log(LogType type, std::string_view str) = 0;
protected:
	inline ~ILogger() = default;
};

} // namespace Ignis::Multirole::Core

#endif // ILOGGER_HPP
