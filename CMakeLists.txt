cmake_minimum_required(VERSION 3.12.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()

option(IGNIS_MULTIROLE_MSVC_STATIC "Static CRT on Visual Studio" ON)

project(ignis-multirole)
add_executable(multirole)
set_property (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "multirole")

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
