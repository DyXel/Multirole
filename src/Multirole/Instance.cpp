#include "Instance.hpp"

#include <csignal>
#include <cstdlib> // Exit flags
#include <thread>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/json/value.hpp>

#define LOG_INFO(...) logHandler.Log(ServiceType::MULTIROLE, Level::INFO, __VA_ARGS__)
#include "I18N.hpp"

namespace Ignis::Multirole
{

namespace
{

constexpr unsigned int GetConcurrency(int hint) noexcept
{
	if(hint <= 0)
		return std::max(1U, std::thread::hardware_concurrency() * 2U);
	return static_cast<unsigned int>(hint);
}

inline Service::CoreProvider::CoreType GetCoreType(std::string_view str)
{
	auto ret = Service::CoreProvider::CoreType::SHARED;
	if(str == "hornet")
		ret = Service::CoreProvider::CoreType::HORNET;
	else if(str != "shared")
		throw std::runtime_error(I18N::MULTIROLE_INCORRECT_CORE_TYPE);
	return ret;
}

} // namespace

// public

Instance::Instance(const boost::json::value& cfg) :
	auxIoCtx(),
	lIoCtx(),
	lIoCtxGuard(boost::asio::make_work_guard(lIoCtx)),
	hostingConcurrency(GetConcurrency(cfg.at("concurrencyHint").to_number<int>())),
	logHandler(auxIoCtx, cfg.at("logHandler").as_object()),
	banlistProvider(logHandler, cfg.at("banlistProvider").at("fileRegex").as_string()),
	coreProvider(
		logHandler,
		cfg.at("coreProvider").at("fileRegex").as_string(),
		cfg.at("coreProvider").at("tmpPath").as_string().data(),
		GetCoreType(cfg.at("coreProvider").at("coreType").as_string()),
		cfg.at("coreProvider").at("loadPerRoom").as_bool()),
	dataProvider(logHandler, cfg.at("dataProvider").at("fileRegex").as_string()),
	replayManager(
		logHandler,
		cfg.at("replayManager").at("save").as_bool(),
		cfg.at("replayManager").at("path").as_string().data()),
	scriptProvider(logHandler, cfg.at("scriptProvider").at("fileRegex").as_string()),
	service({banlistProvider, coreProvider, dataProvider, logHandler,
		replayManager, scriptProvider}),
	lobby(cfg.at("lobbyMaxConnections").to_number<int>()),
	lobbyListing(
		lIoCtx,
		cfg.at("lobbyListingPort").to_number<unsigned short>(),
		lobby),
	roomHosting(
		lIoCtx,
		service,
		lobby,
		cfg.at("roomHostingPort").to_number<unsigned short>()),
	signalSet(lIoCtx)
{
	// Load up and update repositories while also adding them to the std::map
	for(const auto& opts : cfg.at("repos").as_array())
	{
		auto name = opts.at("name").as_string().data();
		LOG_INFO(I18N::MULTIROLE_ADDING_REPO, name);
		repos.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(logHandler, auxIoCtx, opts));
	}
	// Register respective providers on their observed repositories
	auto RegRepos = [&](IGitRepoObserver& obs, const boost::json::value& v)
	{
		for(const auto& observed : v.at("observedRepos").as_array())
			repos.at(observed.as_string().data()).AddObserver(obs);
	};
	RegRepos(dataProvider, cfg.at("dataProvider"));
	RegRepos(scriptProvider, cfg.at("scriptProvider"));
	RegRepos(banlistProvider, cfg.at("banlistProvider"));
	RegRepos(coreProvider, cfg.at("coreProvider"));
	// Register signal
	LOG_INFO(I18N::MULTIROLE_SETUP_SIGNAL);
	signalSet.add(SIGTERM);
	signalSet.async_wait([this](std::error_code /*unused*/, int /*unused*/)
	{
		LOG_INFO(I18N::MULTIROLE_SIGNAL_RECEIVED);
		Stop();
	});
	LOG_INFO(I18N::MULTIROLE_HOSTING_THREADS_NUM, hostingConcurrency);
	LOG_INFO(I18N::MULTIROLE_INIT_SUCCESS);
}

int Instance::Run() noexcept
{
	std::thread webhooks([&]{auxIoCtx.run();});
	boost::asio::thread_pool threads(hostingConcurrency);
	for(unsigned int i = 0U; i < hostingConcurrency; i++)
		boost::asio::dispatch(threads, [&]{lIoCtx.run();});
	webhooks.join();
	threads.join();
	LOG_INFO(I18N::MULTIROLE_GOODBYE);
	return EXIT_SUCCESS;
}

// private

void Instance::Stop() noexcept
{
	LOG_INFO(I18N::MULTIROLE_CLEANING_UP);
	auxIoCtx.stop(); // Finishes execution of thread created in Instance::Run
	lIoCtxGuard.reset(); // Allows hosting threads to finish execution
	repos.clear(); // Closes repositories (so other process can acquire locks)
	lobbyListing.Stop();
	roomHosting.Stop();
	if(const std::size_t remainingRooms = lobby.Close(); remainingRooms > 0U)
		LOG_INFO(I18N::MULTIROLE_REMAINING_ROOMS, remainingRooms);
}

} // namespace Ignis::Multirole
