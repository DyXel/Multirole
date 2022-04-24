#ifndef MULTIROLE_SERVICE_LOGHANDLER_SINKARGS_HPP
#define MULTIROLE_SERVICE_LOGHANDLER_SINKARGS_HPP
#include <tuple>
#include <variant>

#include "Constants.hpp"

namespace Ignis::Multirole::LogHandlerDetail
{

using SvcLogProps = std::pair<ServiceType, Level>;
using ECLogProps = std::tuple<ErrorCategory, uint64_t /*replayId*/, uint32_t /*turnCounter*/>;
using SinkLogProps = std::variant<SvcLogProps, ECLogProps>;

} // namespace Ignis::Multirole::LogHandlerDetail

#endif // MULTIROLE_SERVICE_LOGHANDLER_SINKARGS_HPP
