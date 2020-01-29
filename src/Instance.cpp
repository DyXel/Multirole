#include "Instance.hpp"

#include <csignal>
#include <cstdlib> // Exit flags
#include <fstream>
#include <future>

#include <fmt/printf.h>

namespace Ignis
{

namespace Multirole {

nlohmann::json LoadConfigJson(std::string_view path)
{
	fmt::print("Loading up config.json...\n");
	std::ifstream i(path.data());
	return nlohmann::json::parse(i);
}

// public

Instance::Instance() :
	lIoCtx(),
	wsIoCtx(),
	cfg(LoadConfigJson("config.json")),
	lobby(),
	lle(lIoCtx, cfg["lobbyListingPort"].get<unsigned short>(), lobby),
	rhe(lIoCtx, cfg["roomHostingPort"].get<unsigned short>(), lobby),
	signalSet(lIoCtx)
{
	fmt::print("Setting up signal handling...\n");
	signalSet.add(SIGINT);
	signalSet.add(SIGTERM);
	signalSet.async_wait([this](const std::error_code&, int sigNum)
	{
		const char* sigName;
		switch(sigNum)
		{
			case SIGINT: sigName = "SIGINT"; break;
			case SIGTERM: sigName = "SIGTERM"; break;
			default: sigName = "Unknown signal"; break;
		}
		fmt::print("{} received.\n", sigName);
		Stop();
	});
}

int Instance::Run()
{
	std::future<std::size_t> wsHExec = std::async(std::launch::async,
	[this]()
	{
		return wsIoCtx.run();
	});
	// This call will only return after all connections are properly closed
	std::size_t tHExec = lIoCtx.run();
	tHExec += wsHExec.get();
	fmt::print("All Contexts stopped. Total handlers executed: {}\n", tHExec);
	return EXIT_SUCCESS;
}

// private

void Instance::Stop()
{
	fmt::print("Closing all acceptors and finishing IO operations...\n");
	wsIoCtx.stop(); // Terminates thread
	lle.Stop();
	rhe.Stop();
	const auto startedRoomsCount = lobby.GetStartedRoomsCount();
	lobby.StopNonStartedRooms(); // implicitly changes WAITING rooms to STOPPING
	if(startedRoomsCount > 0u)
	{
		fmt::print("All done, server will gracefully finish execution\n");
		fmt::print("after all duels finish. If you wish to forcefully end\n");
		fmt::print("you can terminate the process safely now (SIGKILL, etc)\n");
		fmt::print("Number of current duels: {}\n", startedRoomsCount);
	}
}

} // namespace Multirole

} // namespace Ignis
