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
#include <git2.h>
#include <fmt/printf.h>
#include "ServerInstance.hpp"

int main()
{
	fmt::print("Project Ignis: Multirole, the robust server for YGOPro\n");
	int exitFlag = EXIT_SUCCESS;
	git_libgit2_init(); // TODO: check for initialization failure?
	if(exitFlag == EXIT_SUCCESS)
	{
		try
		{
			Ignis::ServerInstance server;
			exitFlag = server.Run();
			fmt::print("Server context finished execution\n");
		}
		catch(const std::exception& e)
		{
			fmt::print("Error: {}\n\n", e.what());
			exitFlag = EXIT_FAILURE;
		}
	}
	git_libgit2_shutdown();
	return exitFlag;
}
