#include "FileSystem.hpp"

namespace FileSystem
{

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

bool MakeDir(std::string_view path)
{
	return CreateDirectoryA(path.data(), NULL) || ERROR_ALREADY_EXISTS == GetLastError();
}

#else
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>

bool MakeDir(std::string_view path)
{
	return mkdir(path.data(), 0777) == 0 || errno == EEXIST;
}

#endif

} // namespace FileSystem
