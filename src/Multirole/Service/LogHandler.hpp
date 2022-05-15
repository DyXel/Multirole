#ifndef SERVICE_LOGHANDLER_HPP
#define SERVICE_LOGHANDLER_HPP
#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string_view>

#include <boost/asio/io_context.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/json/object.hpp>
#include <fmt/format.h>

#include "../Service.hpp"
#include "LogHandler/Constants.hpp"

namespace Ignis::Multirole
{

namespace LogHandlerDetail
{

class ISink;

} // namespace LogHandlerDetail

class RoomLogger;

class Service::LogHandler
{
public:
	LogHandler(boost::asio::io_context& ioCtx, const boost::json::object& cfg);
	~LogHandler();

	std::unique_ptr<RoomLogger> MakeRoomLogger(uint32_t roomId) const noexcept;

	void Log(ServiceType svc, Level lvl, std::string_view str) const noexcept;
	void Log(ErrorCategory cat, uint64_t replayId, uint32_t turnCounter, std::string_view str) const noexcept;

	// Helper overloads used to format stuff nicely.
	template<typename... Args>
	inline void Log(ServiceType svc, Level lvl, std::string_view str, Args&& ...args) const noexcept
	{
		Log(svc, lvl, fmt::format(str, std::forward<Args&&>(args)...));
	}

	template<typename... Args>
	inline void Log(ErrorCategory cat, uint64_t replayId, uint32_t turnCounter, std::string_view str, Args&& ...args) const noexcept
	{
		Log(cat, replayId, turnCounter, fmt::format(str, std::forward<Args&&>(args)...));
	}
private:
	const bool logRooms;
	const boost::filesystem::path roomLogsDir;
	std::mutex mStderr;
	std::mutex mStdout;
	std::array<
		std::unique_ptr<LogHandlerDetail::ISink>,
		static_cast<std::size_t>(ServiceType::SERVICE_TYPE_COUNT)
	> serviceSinks;
	std::array<
		std::unique_ptr<LogHandlerDetail::ISink>,
		static_cast<std::size_t>(ErrorCategory::ERROR_CATEGORY_COUNT)
	> ecSinks;
};

class RoomLogger
{
public:
	RoomLogger(const boost::filesystem::path& path);

	void Log(std::string_view str) noexcept;

	// Helper overload used to format stuff nicely.
	template<typename... Args>
	inline void Log(std::string_view str, Args&& ...args) noexcept
	{
		Log(fmt::format(str, std::forward<Args&&>(args)...));
	}
private:
	std::ofstream f;
};

} // namespace Ignis::Multirole

#endif // SERVICE_LOGHANDLER_HPP
