cmake_minimum_required(VERSION 3.12.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()

option(IGNIS_MULTIROLE_MSVC_STATIC "Static CRT on Visual Studio" ON)
option(IGNIS_MULTIROLE_SYSTEM_DEPS "Use system dependencies" OFF)

project(ignis-multirole)
add_executable(multirole)
set_property (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "multirole")

if(NOT IGNIS_MULTIROLE_SYSTEM_DEPS)
	find_package(asio CONFIG REQUIRED)
	find_package(fmt CONFIG REQUIRED)
	find_package(libgit2 REQUIRED)
	find_package(nlohmann_json CONFIG REQUIRED)
	find_package(sqlite3 REQUIRED)
	# libgit2 dependencies
	if(NOT WIN32)
		find_package(OpenSSL REQUIRED)
	endif()
	find_package(ZLIB REQUIRED)

	target_include_directories(multirole PRIVATE
		${ASIO_INCLUDE_DIR}
		${LIBGIT2_INCLUDE_DIRS}
		${SQLite3_INCLUDE_DIR}
	)
endif()

# Doesn't get set on Linux
if(NOT DEFINED SQLite3_LIBRARY)
	set(SQLite3_LIBRARY sqlite3)
endif()

if(NOT DEFINED LIBGIT2_LIBRARIES)
	set(LIBGIT2_LIBRARIES git2)
endif()

target_link_libraries(multirole PRIVATE
	${LIBGIT2_LIBRARIES}
	${SQLite3_LIBRARY}
	${CMAKE_THREAD_LIBS_INIT}
)
if(NOT IGNIS_MULTIROLE_SYSTEM_DEPS)
	target_link_libraries(multirole PRIVATE
		asio::asio
		fmt::fmt
		nlohmann_json::nlohmann_json
		ZLIB::ZLIB
	)
	if(NOT WIN32)
		target_link_libraries(multirole PRIVATE OpenSSL::SSL OpenSSL::Crypto)
	endif()
endif()

if(WIN32)
	target_link_libraries(multirole PRIVATE crypt32 rpcrt4 winhttp)
else()
	target_link_libraries(multirole PRIVATE ${CMAKE_DL_LIBS})
endif()

add_subdirectory(src)

if(WIN32)
	add_compile_definitions(UNICODE)
endif()

set(CLANG_OR_GNU $<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>)
target_compile_options(multirole PRIVATE
	# Clang and GNU compilers
	$<${CLANG_OR_GNU}:-Wall -Wextra -pedantic>
	$<$<AND:$<CONFIG:Debug>,${CLANG_OR_GNU}>:-ggdb3 -Og -D_DEBUG>
	$<$<AND:$<CONFIG:Release>,${CLANG_OR_GNU}>:-O3>
	$<$<AND:$<CONFIG:RelWithDebInfo>,${CLANG_OR_GNU}>:-ggdb3 -O3 -D_DEBUG>
	# Microsoft compiler
	$<$<CXX_COMPILER_ID:MSVC>:/W4>
)

if(IGNIS_MULTIROLE_MSVC_STATIC AND MSVC)
	set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )
	foreach(CompilerFlag ${CompilerFlags})
  		string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
	endforeach()
	# CMake 3.15
	# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
