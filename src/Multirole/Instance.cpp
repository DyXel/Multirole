#include "Instance.hpp"

#include <csignal>
#include <cstdlib> // Exit flags
#include <fstream>
#include <future>

#include <spdlog/spdlog.h>

namespace Ignis::Multirole
{

nlohmann::json LoadConfigJson(std::string_view path)
{
	spdlog::info("Loading up config.json...");
	std::ifstream i(path.data());
	return nlohmann::json::parse(i);
}

// public

Instance::Instance() :
	lIoCtx(),
	whIoCtx(),
	cfg(LoadConfigJson("config.json")),
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
		CoreProvider::CoreType type;
		if(coreTypeStr == "shared")
			type = CoreProvider::CoreType::SHARED;
		else if(coreTypeStr == "hornet")
			type = CoreProvider::CoreType::HORNET;
		else
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
		const char* sigName;
		switch(sigNum)
		{
			case SIGINT: sigName = "SIGINT"; break;
			case SIGTERM: sigName = "SIGTERM"; break;
			default: sigName = "Unknown signal"; break;
		}
		spdlog::info("{:s} received.", sigName);
		Stop();
	});
	spdlog::info("Initialization finished successfully!");
}

int Instance::Run()
{
	std::future<std::size_t> wsHExec = std::async(std::launch::async,
	[this]()
	{
		return whIoCtx.run();
	});
	// Next run call will only return after all connections are properly closed
	std::size_t tHExec = lIoCtx.run();
	tHExec += wsHExec.get();
	spdlog::info("Total handlers executed: {}", tHExec);
	return EXIT_SUCCESS;
}

// private

constexpr const char* UNFINISHED_DUELS_STRING =
R"(
All done, server will gracefully finish execution
after all duels finish. If you wish to forcefully end
you can terminate the process safely now (SIGKILL)
)";

void Instance::Stop()
{
	spdlog::info("Closing all acceptors and finishing IO operations...");
	whIoCtx.stop(); // Terminates thread
	repos.clear();
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
