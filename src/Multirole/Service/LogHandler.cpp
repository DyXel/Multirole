#include "LogHandler.hpp"

#include <boost/filesystem.hpp>

#include "../I18N.hpp"
#include "LogHandler/ISink.hpp"
#include "LogHandler/DiscordWebhookSink.hpp"
#include "LogHandler/FileSink.hpp"
#include "LogHandler/StderrSink.hpp"
#include "LogHandler/StdoutSink.hpp"

namespace Ignis::Multirole
{

namespace
{

using Str = const char* const;

template<typename JsonValue>
constexpr Str AsStr(const JsonValue& jv)
{
	return jv.as_string().data();
};

} // namespace

Service::LogHandler::LogHandler(boost::asio::io_context& ioCtx, const boost::json::object& cfg) :
	logRooms(cfg.at("roomLogging").at("enabled").as_bool()),
	roomLogsDir(AsStr(cfg.at("roomLogging").at("path"))),
	mStderr(),
	mStdout()
{
	using namespace LogHandlerDetail;
	constexpr Str SERVICE_SINKS = "serviceSinks";
	constexpr Str EC_SINKS = "ecSinks";
	auto MakeOneSink = [&](Str category, Str name) -> std::unique_ptr<ISink>
	{
		const auto& obj = cfg.at(category).at(name);
		const auto& type = obj.at("type").as_string();
		const auto& props = obj.at("properties").as_object();
		const auto ridFormat = [&]() -> Str
		{
			if(const auto s = props.find("ridFormat"); s != props.cend())
				return AsStr(s->value());
			else
				return "\nReplay ID: {0}";
		}();
		if(type == "discordWebhook")
			return std::make_unique<DiscordWebhookSink>(ioCtx, AsStr(props.at("uri")), ridFormat);
		if(type == "file")
			return std::make_unique<FileSink>(AsStr(props.at("path")));
		if(type == "stderr")
			return std::make_unique<StderrSink>(mStderr);
		if(type == "stdout")
			return std::make_unique<StdoutSink>(mStdout);
		if(type == "null")
			return std::make_unique<ISink>();
		throw std::runtime_error("Wrong type of sink!");
	};
	serviceSinks =
	{
		MakeOneSink(SERVICE_SINKS, "gitRepo"),
		MakeOneSink(SERVICE_SINKS, "multirole"),
		MakeOneSink(SERVICE_SINKS, "banlistProvider"),
		MakeOneSink(SERVICE_SINKS, "coreProvider"),
		MakeOneSink(SERVICE_SINKS, "dataProvider"),
		MakeOneSink(SERVICE_SINKS, "logHandler"),
		MakeOneSink(SERVICE_SINKS, "replayManager"),
		MakeOneSink(SERVICE_SINKS, "scriptProvider"),
	};
	ecSinks =
	{
		MakeOneSink(EC_SINKS, "core"),
		MakeOneSink(EC_SINKS, "official"),
		MakeOneSink(EC_SINKS, "speed"),
		MakeOneSink(EC_SINKS, "rush"),
		MakeOneSink(EC_SINKS, "other"),
	};
	if(!logRooms)
		return;
	using namespace boost::filesystem;
	if(!exists(roomLogsDir) && !create_directory(roomLogsDir))
		throw std::runtime_error(I18N::LOG_HANDLER_COULD_NOT_CREATE_DIR);
	if(!is_directory(roomLogsDir))
		throw std::runtime_error(I18N::LOG_HANDLER_PATH_IS_FILE_NOT_DIR);
}

Service::LogHandler::~LogHandler() = default;

void Service::LogHandler::Log(ServiceType svc, Level lvl, std::string_view str) const noexcept
{
	using namespace LogHandlerDetail;
	const auto& sink = serviceSinks[static_cast<std::size_t>(svc)];
	sink->Log(TimestampNow(), SvcLogProps{svc, lvl}, str);
}

void Service::LogHandler::Log(ErrorCategory cat, uint64_t replayId, std::string_view str) const noexcept
{
	using namespace LogHandlerDetail;
	const auto& sink = ecSinks[static_cast<std::size_t>(cat)];
	sink->Log(TimestampNow(), ECLogProps{cat, replayId}, str);
}

std::unique_ptr<RoomLogger> Service::LogHandler::MakeRoomLogger(uint32_t roomId) const noexcept
{
	if(!logRooms)
		return nullptr;
	try
	{
		const auto cnt = LogHandlerDetail::TimestampNow().time_since_epoch().count();
		const auto fn = fmt::format("{}-{}.log", cnt, roomId);
		return std::make_unique<RoomLogger>(roomLogsDir / fn);
	}
	catch(const std::exception& e)
	{
		Log(ServiceType::LOG_HANDLER, Level::ERROR, I18N::LOG_HANDLER_CANNOT_CREATE_ROOM_LOGGER, e.what());
		return nullptr;
	}
}

// RoomLogger

RoomLogger::RoomLogger(const boost::filesystem::path& path) :
	f(path)
{
	if(!f.is_open())
		throw std::runtime_error(I18N::ROOM_LOGGER_FILE_IS_NOT_OPEN);
}

void RoomLogger::Log(std::string_view str) noexcept
{
	LogHandlerDetail::FmtTimestamp(f, LogHandlerDetail::TimestampNow());
	f << ' ' << str << '\n';
}

} // namespace Ignis::Multirole
