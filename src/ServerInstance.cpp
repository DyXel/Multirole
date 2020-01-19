#include "ServerInstance.hpp"

#include <cstdio> // std::puts -- TODO: Use a different log system?
#include <cstdlib> // Exit flags

#include "SharedCore.hpp"

namespace Placeholder4
{

ServerInstance::ServerInstance()
{
	std::puts("Project Ignis: Multirole, the robust server for YGOPro");

	core = std::make_shared<SharedCore>("./libocgcore.so");

	int major, minor;
	core->OCG_GetVersion(&major, &minor);
	std::printf("Core version (%i, %i)\n", major, minor);
}

int ServerInstance::Run()
{
	return EXIT_SUCCESS;
}

} // namespace Placeholder4
