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

// public

Instance::Instance(const nlohmann::json& cfg) :
	whIoCtx(),
	lIoCtx(),
	lIoCtxGuard(asio::make_work_guard(lIoCtx)),
	hostingConcurrency(
		GetConcurrency(cfg.at("concurrencyHint").get<int>())),
	dataProvider(
		cfg.at("dataProvider").at("fileRegex").get<std::string>()),
	scriptProvider(
		cfg.at("scriptProvider").at("fileRegex").get<std::string>()),
	coreProvider(
		cfg.at("coreProvider").at("fileRegex").get<std::string>(),
		dataProvider,
		scriptProvider),
	banlistProvider(
		cfg.at("banlistProvider").at("fileRegex").get<std::string>()),
	lobbyListing(
		lIoCtx,
		cfg.at("lobbyListingPort").get<unsigned short>(), lobby),
	roomHosting(
		lIoCtx,
		cfg.at("roomHostingPort").get<unsigned short>(),
		coreProvider,
		banlistProvider,
		lobby),
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
	// Set up coreProvider
	{
		const auto& cpProps = cfg.at("coreProvider");
		const auto& coreTypeStr = cpProps.at("coreType").get<std::string>();
		const auto loadPerRoom = cpProps.at("loadPerRoom").get<bool>();
		CoreProvider::CoreType type = CoreProvider::CoreType::SHARED;
		if(coreTypeStr == "hornet")
			type = CoreProvider::CoreType::HORNET;
		else if(coreTypeStr != "shared")
			throw std::runtime_error("Incorrect type of core.");
		coreProvider.SetLoadProperties(type, loadPerRoom);
		RegRepos(coreProvider, cfg.at("coreProvider").at("observedRepos"));
	}
	// Register signals
	spdlog::info("Setting up signal handling...");
	signalSet.add(SIGINT);
	signalSet.add(SIGTERM);
	signalSet.async_wait([this](const std::error_code& /*unused*/, int sigNum)
	{
		const char* sigName = nullptr;
		switch(sigNum)
		{
			case SIGINT: sigName = "SIGINT"; break;
			case SIGTERM: sigName = "SIGTERM"; break;
			default: sigName = "Unknown signal"; break;
		}
		spdlog::info("{:s} received.", sigName);
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
" you can terminate the process safely now (SIGKILL)";

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
