#include "ServerInstance.hpp"

#include <csignal>
#include <cstdlib> // Exit flags

#include <asio.hpp>
#include <fmt/printf.h>

namespace Ignis
{

// public

ServerInstance::ServerInstance() : signalSet(ioContext)
{
	fmt::print("Project Ignis: Multirole, the robust server for YGOPro\n");

	// Setup signal handling
	signalSet.add(SIGINT);
	signalSet.add(SIGTERM);
	signalSet.async_wait([this](const std::error_code& ec, int sigNum)
	{
		if(ec)
			fmt::print("Error on signal processing: {}\n", ec.message());
		// Print signal received
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
	ioContext.stop();
}

} // namespace Ignis
