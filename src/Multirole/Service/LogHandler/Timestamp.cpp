#include "Timestamp.hpp"

#include <array>
#include <ctime>

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
	const auto* lt = std::localtime(&tt);
	std::array<char, 32U> tBuf{};
	const auto sz1 = std::strftime(tBuf.data(), tBuf.size(), "%Y-%m-%d %T", lt);
	std::array<char, 4U> msBuf{};
	const auto ms = static_cast<unsigned>(duration_cast<milliseconds>(timestamp.time_since_epoch()).count() % 1000U);
	const auto sz2 = std::snprintf(msBuf.data(), msBuf.size(), "%03u", ms);
	os.put('[').write(tBuf.data(), sz1).put('.').write(msBuf.data(), sz2).put(']');
}

} // namespace Ignis::Multirole::LogHandlerDetail
