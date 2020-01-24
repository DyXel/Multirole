#include "LoggerToStdout.hpp"

#include <string>
#include <fmt/printf.h>

namespace Ignis
{

namespace Multirole
{

LoggerToStdout::LoggerToStdout(asio::io_context& ioContext) : strand(ioContext)
{}

void LoggerToStdout::Log(std::string_view str)
{
	std::string strObj(str);
	asio::post(strand,
	[strObj = std::move(strObj)]()
	{
		fmt::print(strObj);
	});
}

void LoggerToStdout::LogError(std::string_view str)
{
	std::string strObj(str);
	asio::post(strand,
	[strObj = std::move(strObj)]()
	{
		fmt::print("Error: {}", strObj);
	});
}

} // namespace Multirole

} // namespace Ignis
