/**
 *  Project Ignis: Multirole
 *  Copyright (C) 2020  DyXel, edo9300, kevinlul
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <cstdlib> // Exit flags
#include <fstream> // std::ifstream
#include <optional> // std::optional

#include <fmt/printf.h>
#include <spdlog/spdlog.h>
#include <git2.h>

#include "Instance.hpp"

static constexpr const char* NOTICE_STRING =
R"(Project Ignis: Multirole
Copyright (C) 2020  DyXel, edo9300, kevinlul
)";

inline int CreateAndRunServerInstance()
{
	std::optional<Ignis::Multirole::Instance> server;
	try
	{
		std::ifstream f("multirole-config.json");
		const auto cfg = nlohmann::json::parse(f);
		server.emplace(cfg);
	}
	catch(const std::exception& e)
	{
		fmt::print("Error while initializing server: {:s}\n", e.what());
		return EXIT_FAILURE;
	}
	return server->Run();
}

int main()
{
	fmt::print(NOTICE_STRING);
	git_libgit2_init();
	int exitFlag = CreateAndRunServerInstance();
	spdlog::shutdown();
	git_libgit2_shutdown();
	return exitFlag;
}
