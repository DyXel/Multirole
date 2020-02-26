#include "LoggerToStdout.hpp"

#include <string>
#include <ctime>

#include <asio/post.hpp>
#include <fmt/printf.h>

namespace Ignis
{

namespace Multirole
{

LoggerToStdout::LoggerToStdout() :
	guard(asio::make_work_guard(ioCtx)),
	t([this](){ioCtx.run();})
{}

LoggerToStdout::~LoggerToStdout()
{
	// Allow ioCtx.run() to return and wait for thread to finish
	guard.reset();
	t.join();
}

void LoggerToStdout::Log(std::string_view str)
{
	std::string strObj(str);
	asio::post(ioCtx,
	[time = FormattedTime(), strObj = std::move(strObj)]()
	{
		fmt::print(FMT_STRING("[{:s}] Info: {:s}\n"), time, strObj);
	});
}

void LoggerToStdout::LogError(std::string_view str)
{
	std::string strObj(str);
	asio::post(ioCtx,
	[time = FormattedTime(), strObj = std::move(strObj)]()
	{
		fmt::print(FMT_STRING("[{:s}] Error: {:s}\n"), time, strObj);
	});
}

std::string LoggerToStdout::FormattedTime() const
{
	std::time_t t = std::time(nullptr);
	char mbstr[100];
	if(std::strftime(mbstr, sizeof(mbstr), "%c", std::localtime(&t)))
		return std::string(mbstr);
	return std::string("Unknown time");
}

} // namespace Multirole

} // namespace Ignis
