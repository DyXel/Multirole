#include "ServerInstance.hpp"

#include <csignal>
#include <cstdlib> // Exit flags
#include <fstream>

#include <fmt/printf.h>

namespace Ignis
{

nlohmann::json LoadConfigJson(std::string_view path)
{
	fmt::print("Loading up config.json...\n");
	std::ifstream i(path.data());
	return nlohmann::json::parse(i);
}

// public

ServerInstance::ServerInstance() :
	lobbyIoContext(),
	cfg(LoadConfigJson("config.json")),
	lobby(),
	lle(lobbyIoContext, cfg["lobbyListPort"].get<unsigned short>(), lobby),
	signalSet(lobbyIoContext)
{
	fmt::print("Setting up signal handling...\n");
	signalSet.add(SIGINT);
	signalSet.add(SIGTERM);
	DoWaitSignal();
}

int ServerInstance::Run()
{
	const std::size_t hExec = lobbyIoContext.run();
	fmt::print("Context stopped. Total handlers executed: {}\n", hExec);
	return EXIT_SUCCESS;
}

// private

void ServerInstance::DoWaitSignal()
{
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
		Terminate();
	});
}

void ServerInstance::Stop()
{
	lle.Stop();
}

void ServerInstance::Terminate()
{
	fmt::print("Finishing Execution...\n");
	lle.Terminate();
	lobbyIoContext.stop();
}

} // namespace Ignis
