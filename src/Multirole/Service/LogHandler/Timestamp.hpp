#ifndef MULTIROLE_SERVICE_LOGHANDLER_TIMESTAMP_HPP
#define MULTIROLE_SERVICE_LOGHANDLER_TIMESTAMP_HPP
#include <chrono>
#include <ostream>

namespace Ignis::Multirole::LogHandlerDetail
{

using Timestamp = std::chrono::time_point<std::chrono::system_clock>;

Timestamp TimestampNow() noexcept;
void FmtTimestamp(std::ostream& os, const Timestamp& timestamp) noexcept;

} // namespace Ignis::Multirole::LogHandlerDetail

#endif // MULTIROLE_SERVICE_LOGHANDLER_TIMESTAMP_HPP
