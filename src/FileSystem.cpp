#include "FileSystem.hpp"

namespace FileSystem
{

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

bool DoesDirExist(std::string_view path)
{
	auto dwAttrib = GetFileAttributesA(path.data());
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool MakeDir(std::string_view path)
{
	return CreateDirectoryA(path.data(), NULL) || ERROR_ALREADY_EXISTS == GetLastError();
}

bool DoesFileExist(std::string_view path)
{
	auto dwAttrib = GetFileAttributesA(path.data());
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

#else
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

bool DoesDirExist(std::string_view path)
{
	DIR* dir = opendir(path.data());
	if(dir)
	{
		closedir(dir);
		return true;
	}
	return false;
}

bool MakeDir(std::string_view path)
{
	return !mkdir(path.data(), 0777) || errno == EEXIST;
}

bool DoesFileExist(std::string_view path)
{
	struct stat sb;
	if(stat(path.data(), &sb) == -1)
		return false;
	return S_ISREG(sb.st_mode) != 0;
}

#endif

} // namespace FileSystem
