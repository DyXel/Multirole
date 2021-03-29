#ifndef MULTIROLE_SERVICE_LOGHANDLER_STDERRSINK_HPP
#define MULTIROLE_SERVICE_LOGHANDLER_STDERRSINK_HPP
#include "ISink.hpp"

#include <mutex>

namespace Ignis::Multirole::LogHandlerDetail
{

class StderrSink final : public ISink
{
public:
	StderrSink(std::mutex& mtx);
	~StderrSink() noexcept;
	void Log(const Timestamp& ts, const SinkLogProps& props, std::string_view str) noexcept override;
private:
	std::mutex& mtx;
};

} // namespace Ignis::Multirole::LogHandlerDetail

#endif // MULTIROLE_SERVICE_LOGHANDLER_STDERRSINK_HPP
