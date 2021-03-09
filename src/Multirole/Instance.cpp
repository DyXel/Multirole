#include "Instance.hpp"

#include <csignal>
#include <cstdlib> // Exit flags
#include <thread>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/json/value.hpp>
#include <spdlog/spdlog.h>

namespace Ignis::Multirole
{

constexpr unsigned int GetConcurrency(int hint)
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
		throw std::runtime_error("Incorrect type of core.");
	return ret;
}

// public

Instance::Instance(const boost::json::value& cfg) :
	whIoCtx(),
	lIoCtx(),
	lIoCtxGuard(boost::asio::make_work_guard(lIoCtx)),
	hostingConcurrency(GetConcurrency(cfg.at("concurrencyHint").to_number<int>())),
	banlistProvider(cfg.at("banlistProvider").at("fileRegex").as_string()),
	coreProvider(
		cfg.at("coreProvider").at("fileRegex").as_string(),
		cfg.at("coreProvider").at("tmpPath").as_string(),
		GetCoreType(cfg.at("coreProvider").at("coreType").as_string()),
		cfg.at("coreProvider").at("loadPerRoom").as_bool()),
	dataProvider(cfg.at("dataProvider").at("fileRegex").as_string()),
	replayManager(cfg.at("replaysPath").as_string()),
	scriptProvider(cfg.at("scriptProvider").at("fileRegex").as_string()),
	service({banlistProvider, coreProvider, dataProvider,
		replayManager, scriptProvider}),
	lobby(),
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
		spdlog::info("Adding repository '{:s}'...", name);
		repos.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(whIoCtx, opts));
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
	spdlog::info("Setting up signal handling...");
	signalSet.add(SIGTERM);
	signalSet.async_wait([this](std::error_code /*unused*/, int /*unused*/)
	{
		spdlog::info("SIGTERM received.");
		Stop();
	});
	spdlog::info("Hosting will use {:d} threads", hostingConcurrency);
	spdlog::info("Initialization finished successfully!");
}

int Instance::Run()
{
	std::thread webhooks([&]{whIoCtx.run();});
	boost::asio::thread_pool threads(hostingConcurrency);
	for(unsigned int i = 0U; i < hostingConcurrency; i++)
		boost::asio::dispatch(threads, [&]{lIoCtx.run();});
	webhooks.join();
	threads.join();
	return EXIT_SUCCESS;
}

// private

constexpr const char* UNFINISHED_DUELS_STRING =
"All done, server will gracefully finish execution"
" after all duels finish. If you wish to forcefully end"
" you can terminate the process safely now (SIGTERM/SIGKILL)";

void Instance::Stop()
{
	spdlog::info("Closing all acceptors and finishing IO operations...");
	whIoCtx.stop(); // Finishes execution of thread created in Instance::Run
	lIoCtxGuard.reset(); // Allows hosting threads to finish execution
	repos.clear(); // Closes repositories (so other process can acquire locks)
	lobbyListing.Stop();
	roomHosting.Stop();
	const auto startedRoomsCount = lobby.GetStartedRoomsCount();
	lobby.CloseNonStartedRooms();
	if(startedRoomsCount > 0U)
	{
		spdlog::info(UNFINISHED_DUELS_STRING);
		spdlog::info("Remaining rooms: {:d}", startedRoomsCount);
	}
}

} // namespace Ignis::Multirole
