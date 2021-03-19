#include "Timestamp.hpp"

namespace Ignis::Multirole::LogHandlerDetail
{

Timestamp TimestampNow() noexcept
{
	return std::chrono::system_clock::now();
}

void FmtTimestamp(std::ostream& os, const Timestamp& timestamp) noexcept
{
	const auto t = std::chrono::system_clock::to_time_t(timestamp);
	const auto gmtime = std::gmtime(&t);
	char buffer[32];
	strftime(buffer, 32, "%Y-%m-%d %T", gmtime);
	os << '[' << buffer << ']';
}

} // namespace Ignis::Multirole::LogHandlerDetail
