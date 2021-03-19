#ifndef MULTIROLE_SERVICE_LOGHANDLER_ISINK_HPP
#define MULTIROLE_SERVICE_LOGHANDLER_ISINK_HPP
#include "SinkArgs.hpp"
#include "Timestamp.hpp"

namespace Ignis::Multirole::LogHandlerDetail
{

class ISink
{
public:
	virtual void Log(const Timestamp& ts, const SinkLogProps& props, std::string_view str) noexcept
	{};
	virtual ~ISink() noexcept = default;
};

} // namespace Ignis::Multirole::LogHandlerDetail

#endif // MULTIROLE_SERVICE_LOGHANDLER_ISINK_HPP
