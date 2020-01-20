#include "ServerInstance.hpp"

#include <csignal>
#include <cstdlib> // Exit flags

#include <asio.hpp>
#include <fmt/printf.h>

#include "Lobby.hpp"
#include "LobbyListEndpoint.hpp"

namespace Ignis
{

// public

ServerInstance::ServerInstance() : signalSet(ioContext)
{
	fmt::print("Project Ignis: Multirole, the robust server for YGOPro\n");

	// Load up configuration
	// TODO

	// Setup signal handling
	signalSet.add(SIGINT);
	signalSet.add(SIGTERM);
	signalSet.async_wait([this](const std::error_code& ec, int sigNum)
	{
		// Print signal received
		const char* sigName;
		switch(sigNum)
		{
			case SIGINT: sigName = "SIGINT"; break;
			case SIGTERM: sigName = "SIGTERM"; break;
			default: sigName = "Unknown signal"; break;
		}
		fmt::print("{} received.\n", sigName);
		if(ec)
			fmt::print("Error on signal processing: {}\n", ec.message());
		Terminate();
	});

	// TODO: Tests to guarantee the server will be able to host duels

	// Start up lobby and both endpoints
	{
		auto lobby = std::make_shared<Lobby>();
		lle = std::make_shared<LobbyListEndpoint>(ioContext, 7922, lobby);
	}
}

int ServerInstance::Run()
{
	const std::size_t hExec = ioContext.run();
	fmt::print("Context stopped. Total handlers executed: {}\n", hExec);
	return EXIT_SUCCESS;
}

// private

void ServerInstance::Terminate()
{
	fmt::print("Finishing Execution...\n");
	lle->Terminate();
	signalSet.cancel();
	ioContext.stop();
}

} // namespace Ignis
