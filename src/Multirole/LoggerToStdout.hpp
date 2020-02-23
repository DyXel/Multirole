#ifndef LOGGERTOSTDOUT_HPP
#define LOGGERTOSTDOUT_HPP
#include <asio/io_context_strand.hpp>

#include "IAsyncLogger.hpp"

namespace Ignis
{

namespace Multirole
{

class LoggerToStdout final : public IAsyncLogger
{
public:
	LoggerToStdout(asio::io_context& ioCtx);
	void Log(std::string_view str) override;
	void LogError(std::string_view str) override;
private:
	asio::io_context::strand strand;

	std::string FormattedTime() const;
};

} // namespace Multirole

} // namespace Ignis

#endif // LOGGERTOSTDOUT_HPP
