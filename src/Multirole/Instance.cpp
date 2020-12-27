#include "Instance.hpp"

#include <csignal>
#include <cstdlib> // Exit flags
#include <thread>

#include <asio/dispatch.hpp>
#include <asio/thread_pool.hpp>
#include <spdlog/spdlog.h>

namespace Ignis::Multirole
{

constexpr unsigned int GetConcurrency(int hint)
{
	if(hint <= 0)
		return std::max(1U, std::thread::hardware_concurrency() * 2U);
	return static_cast<unsigned int>(hint);
}

inline CoreProvider::CoreType GetCoreType(std::string_view str)
{
	CoreProvider::CoreType ret = CoreProvider::CoreType::SHARED;
	if(str == "hornet")
		ret = CoreProvider::CoreType::HORNET;
	else if(str != "shared")
		throw std::runtime_error("Incorrect type of core.");
	return ret;
}

// public

Instance::Instance(const nlohmann::json& cfg) :
	whIoCtx(),
	lIoCtx(),
	lIoCtxGuard(asio::make_work_guard(lIoCtx)),
	hostingConcurrency(
		GetConcurrency(cfg.at("concurrencyHint").get<int>())),
	replayManager(cfg.at("replaysPath").get<std::string>()),
	dataProvider(
		cfg.at("dataProvider").at("fileRegex").get<std::string>()),
	scriptProvider(
		cfg.at("scriptProvider").at("fileRegex").get<std::string>()),
	coreProvider(
		cfg.at("coreProvider").at("fileRegex").get<std::string>(),
		cfg.at("coreProvider").at("tmpPath").get<std::string>(),
		GetCoreType(cfg.at("coreProvider").at("coreType").get<std::string>()),
		cfg.at("coreProvider").at("loadPerRoom").get<bool>()),
	banlistProvider(
		cfg.at("banlistProvider").at("fileRegex").get<std::string>()),
	lobbyListing(
		lIoCtx,
		cfg.at("lobbyListingPort").get<unsigned short>(), lobby),
	roomHosting({
		lIoCtx,
		cfg.at("roomHostingPort").get<unsigned short>(),
		banlistProvider,
		coreProvider,
		dataProvider,
		replayManager,
		scriptProvider,
		lobby}),
	signalSet(lIoCtx)
{
	// Load up and update repositories while also adding them to the std::map
	for(const auto& opts : cfg.at("repos").get<std::vector<nlohmann::json>>())
	{
		auto name = opts.at("name").get<std::string>();
		spdlog::info("Adding repository '{:s}'...", name);
		repos.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(name),
			std::forward_as_tuple(whIoCtx, opts));
	}
	// Register respective providers on their observed repositories
	auto RegRepos = [&](IGitRepoObserver& obs, const nlohmann::json& a)
	{
		for(const auto& observed : a.get<std::vector<std::string>>())
			repos.at(observed).AddObserver(obs);
	};
	RegRepos(dataProvider, cfg.at("dataProvider").at("observedRepos"));
	RegRepos(scriptProvider, cfg.at("scriptProvider").at("observedRepos"));
	RegRepos(banlistProvider, cfg.at("banlistProvider").at("observedRepos"));
	RegRepos(coreProvider, cfg.at("coreProvider").at("observedRepos"));
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
	// NOTE: Needed for overload resolution.
	using RunType = asio::io_context::count_type(asio::io_context::*)();
	constexpr auto run = static_cast<RunType>(&asio::io_context::run);
	std::thread webhooks(std::bind(run, &whIoCtx));
	asio::thread_pool threads(hostingConcurrency);
	for(unsigned int i = 0U; i < hostingConcurrency; i++)
		asio::dispatch(threads, std::bind(run, &lIoCtx));
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
