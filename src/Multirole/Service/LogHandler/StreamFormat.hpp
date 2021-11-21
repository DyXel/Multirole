#ifndef MULTIROLE_SERVICE_LOGHANDLER_STREAMFORMAT_HPP
#define MULTIROLE_SERVICE_LOGHANDLER_STREAMFORMAT_HPP
#include <ostream>
#include <string_view>

#include "SinkArgs.hpp"
#include "Timestamp.hpp"

namespace Ignis::Multirole::LogHandlerDetail
{

// Log format:
// [time] [Service:name] [Level:lvl] str
// [time] [EC:cat] [ReplayID:replayId] str
// Example log:
// [2021-03-18 15:50:44] [Service:ReplayManager] [Level:Error] lastId cannot be opened for reading.
// [2021-03-18 15:50:45] [EC:Core] [ReplayID:2746210] Core exception at processing: Process is not running.
void StreamFormat(std::ostream& os, const Timestamp& ts, const SinkLogProps& props, std::string_view str) noexcept;

} // namespace Ignis::Multirole::LogHandlerDetail

#endif // MULTIROLE_SERVICE_LOGHANDLER_STREAMFORMAT_HPP
