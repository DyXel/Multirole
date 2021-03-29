#ifndef MULTIROLE_SERVICE_LOGHANDLER_STDOUTSINK_HPP
#define MULTIROLE_SERVICE_LOGHANDLER_STDOUTSINK_HPP
#include "ISink.hpp"

#include <mutex>

namespace Ignis::Multirole::LogHandlerDetail
{

class StdoutSink final : public ISink
{
public:
	StdoutSink(std::mutex& mtx);
	~StdoutSink() noexcept;
	void Log(const Timestamp& ts, const SinkLogProps& props, std::string_view str) noexcept override;
private:
	std::mutex& mtx;
};

} // namespace Ignis::Multirole::LogHandlerDetail

#endif // MULTIROLE_SERVICE_LOGHANDLER_STDOUTSINK_HPP
