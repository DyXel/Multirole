#ifndef MULTIROLE_SERVICE_LOGHANDLER_ISINK_HPP
#define MULTIROLE_SERVICE_LOGHANDLER_ISINK_HPP
#include "SinkArgs.hpp"
#include "Timestamp.hpp"

namespace Ignis::Multirole::LogHandlerDetail
{

class ISink
{
public:
	virtual void Log([[maybe_unused]] const Timestamp& ts,
	                 [[maybe_unused]] const SinkLogProps& props,
	                 [[maybe_unused]] std::string_view str) noexcept
	{};
	virtual ~ISink() noexcept = default;
};

} // namespace Ignis::Multirole::LogHandlerDetail

#endif // MULTIROLE_SERVICE_LOGHANDLER_ISINK_HPP
