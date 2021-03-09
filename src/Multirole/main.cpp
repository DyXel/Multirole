/**
 *  Project Ignis: Multirole
 *  Licensed under AGPL
 *  Refer to the COPYING file included.
 */
#include <cstdlib> // Exit flags
#include <fstream> // std::ifstream
#include <optional> // std::optional

#include <boost/json/src.hpp>
#include <spdlog/spdlog.h>
#include <git2.h>

#include "Instance.hpp"

inline int CreateAndRunServerInstance()
{
	std::optional<Ignis::Multirole::Instance> server;
	try
	{
		std::ifstream f("config.json");
		boost::json::monotonic_resource mr;
		boost::json::stream_parser p(&mr);
		for(std::string l; std::getline(f, l);)
			p.write(l);
		p.finish();
		server.emplace(p.release());
	}
	catch(const std::exception& e)
	{
		spdlog::critical("Could not initialize server: {:s}\n", e.what());
		return EXIT_FAILURE;
	}
	return server->Run();
}

int main()
{
	git_libgit2_init();
	int exitFlag = CreateAndRunServerInstance();
	spdlog::shutdown();
	git_libgit2_shutdown();
	return exitFlag;
}
