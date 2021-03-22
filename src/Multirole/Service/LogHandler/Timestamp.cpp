#include "Timestamp.hpp"

namespace Ignis::Multirole::LogHandlerDetail
{

Timestamp TimestampNow() noexcept
{
	return std::chrono::system_clock::now();
}

void FmtTimestamp(std::ostream& os, const Timestamp& timestamp) noexcept
{
	using namespace std::chrono;
	const auto tt = system_clock::to_time_t(timestamp);
	const auto lt = std::localtime(&tt);
	char buffer[32];
	strftime(buffer, 32, "%Y-%m-%d %T", lt);
	char ms_buffer[4];
	const auto ms = static_cast<unsigned>(duration_cast<milliseconds>(timestamp.time_since_epoch()).count() % 1000U);
	sprintf(ms_buffer, "%03u", ms);
	os << '[' << buffer << '.' << ms_buffer << ']';
}

} // namespace Ignis::Multirole::LogHandlerDetail
