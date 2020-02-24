#ifndef LOGGERTOSTDOUT_HPP
#define LOGGERTOSTDOUT_HPP
#include <thread>
#include <asio/io_context.hpp>
#include <asio/executor_work_guard.hpp>

#include "IAsyncLogger.hpp"

namespace Ignis
{

namespace Multirole
{

class LoggerToStdout final : public IAsyncLogger
{
public:
	LoggerToStdout();
	~LoggerToStdout();

	void Log(std::string_view str) override;
	void LogError(std::string_view str) override;
private:
	asio::io_context ioCtx;
	asio::executor_work_guard<decltype(ioCtx)::executor_type> guard;
	std::thread t;

	std::string FormattedTime() const;
};

} // namespace Multirole

} // namespace Ignis

#endif // LOGGERTOSTDOUT_HPP
