#include "LoggerToStdout.hpp"

#include <string>

#include <asio/post.hpp>
#include <fmt/printf.h>

namespace Ignis
{

namespace Multirole
{

LoggerToStdout::LoggerToStdout(asio::io_context& ioCtx) : strand(ioCtx)
{}

void LoggerToStdout::Log(std::string_view str)
{
	std::string strObj(str);
	asio::post(strand,
	[time = FormattedTime(), strObj = std::move(strObj)]()
	{
		fmt::print("[{}] Info: {}\n", time, strObj);
	});
}

void LoggerToStdout::LogError(std::string_view str)
{
	std::string strObj(str);
	asio::post(strand,
	[time = FormattedTime(), strObj = std::move(strObj)]()
	{
		fmt::print("[{}] Error: {}\n", time, strObj);
	});
}

std::string LoggerToStdout::FormattedTime() const
{
	std::time_t t = std::time(nullptr);
	char mbstr[100];
	if(std::strftime(mbstr, sizeof(mbstr), "%A %c", std::localtime(&t)))
		return std::string(mbstr);
	return std::string("Unknown time");
}

} // namespace Multirole

} // namespace Ignis
