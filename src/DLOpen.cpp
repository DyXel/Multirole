#include "DLOpen.hpp"

#include <fmt/printf.h>

// "Native" functions below modified from implementations found in SDL2 library
// Author: Sam Lantinga <slouken@libsdl.org>
// Date: 26-10-2018
// Code version: Lastest version at the date (from hg repo)
// Availability:
//	https://hg.libsdl.org/SDL/raw-file/691c32a30fb9/src/loadso/windows/SDL_sysloadso.c
//	https://hg.libsdl.org/SDL/raw-file/691c32a30fb9/src/loadso/dlopen/SDL_sysloadso.c
// Licensed under: zlib

namespace DLOpen
{

constexpr const char* ESTR_OBJECT = "Failed loading object {:s}: {:s}\n";
constexpr const char* ESTR_FUNCTION = "Failed loading function {:s}: {:s}\n";

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// NOTE: Might want to handle unicode

std::string GetLastErrorStr()
{
	DWORD errorMessageID = GetLastError();
	if(errorMessageID == 0)
		return std::string(); // No error message has been recorded
	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPSTR)&messageBuffer, 0, NULL);
	std::string message(messageBuffer, size);
	LocalFree(messageBuffer);
	return message;
}

void* LoadObject(const char* file)
{
	void* handle = (void*)LoadLibraryA(file);
	if (handle == nullptr)
		fmt::print(FMT_STRING(ESTR_OBJECT), file, GetLastErrorStr());
	return handle;
}

void  UnloadObject(void* handle)
{
	if (handle != nullptr)
		FreeLibrary((HMODULE) handle);
}

void* LoadFunction(void* handle, const char* name)
{
	void* symbol = (void*)GetProcAddress((HMODULE) handle, name);
	if (symbol == nullptr)
		fmt::print(FMT_STRING(ESTR_FUNCTION), name, GetLastErrorStr());
	return symbol;
}

#else
#include <dlfcn.h>

void* LoadObject(const char* file)
{
	void* handle = dlopen(file, RTLD_NOW | RTLD_LOCAL);
	if (handle == nullptr)
		fmt::print(FMT_STRING(ESTR_OBJECT), file, dlerror());
	return handle;
}

void UnloadObject(void* handle)
{
	if (handle != nullptr)
		dlclose(handle);
}

void* LoadFunction(void* handle, const char* name)
{
	void* symbol = dlsym(handle, name);
	if (symbol == nullptr)
	{
		// prepend an underscore for platforms that need that.
		std::string _name = std::string("_") + name;
		symbol = dlsym(handle, _name.c_str());
		if (symbol == nullptr)
			fmt::print(FMT_STRING(ESTR_FUNCTION), name, dlerror());
	}
	return symbol;
}

#endif

} // namespace DLOpen
