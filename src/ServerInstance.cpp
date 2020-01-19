#include "ServerInstance.hpp"

#include <cstdio> // std::puts -- TODO: Use a different log system?
#include <cstdlib> // Exit flags

namespace Placeholder4
{

ServerInstance::ServerInstance()
{
	std::puts("Project Ignis: Multirole, the robust server for YGOPro");
}

int ServerInstance::Run()
{
	return EXIT_SUCCESS;
}

} // namespace Placeholder4
