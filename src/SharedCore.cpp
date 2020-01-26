#include "SharedCore.hpp"

#include <stdexcept> // std::runtime_error
#include <type_traits>

#include <fmt/printf.h>

namespace Ignis
{

namespace Multirole {

// "Native" functions below modified from implementations found in SDL2 library
// Author: Sam Lantinga <slouken@libsdl.org>
// Date: 26-10-2018
// Code version: Lastest version at the date (from hg repo)
// Availability:
//	https://hg.libsdl.org/SDL/raw-file/691c32a30fb9/src/loadso/windows/SDL_sysloadso.c
//	https://hg.libsdl.org/SDL/raw-file/691c32a30fb9/src/loadso/dlopen/SDL_sysloadso.c
// Licensed under: zlib

// Prototypes
void* NativeLoadObject(const char* file);
void* NativeLoadFunction(void* handle, const char* name);
void  NativeUnloadObject(void* handle);

#ifdef _WIN32
#include <windows.h>

// NOTE: Might need to handle unicode

std::string GetLastErrorAsString() {
	//Get the error message, if any.
	DWORD errorMessageID = GetLastError();
	if(errorMessageID == 0)
		return std::string(); //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
								 NULL, errorMessageID, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}

void* NativeLoadObject(const char* file)
{
	void* handle = (void*)LoadLibraryA(file);
	if (handle == nullptr)
		fmt::print("Failed loading DLL {}: {}\n", file, GetLastErrorAsString());
	return handle;
}

void* NativeLoadFunction(void* handle, const char* name)
{
	void* symbol = (void*)GetProcAddress((HMODULE) handle, name);
	if (symbol == nullptr) // TODO: get and print error string
		fmt::print("Failed loading function {}\n", name);
	return symbol;
}

void  NativeUnloadObject(void* handle)
{
	if (handle != nullptr)
		FreeLibrary((HMODULE) handle);
}

#else
#include <dlfcn.h>

void* NativeLoadObject(const char* file)
{
	void* handle;
	handle = dlopen(file, RTLD_NOW|RTLD_LOCAL);
	if (handle == nullptr)
		fmt::print("Failed loading shared object {}: {}\n", file, dlerror());
	return (handle);
}

void* NativeLoadFunction(void* handle, const char* name)
{
	void* symbol = dlsym(handle, name);
	if (symbol == nullptr)
	{
		/* append an underscore for platforms that need that. */
		std::string _name = std::string("_") + name;
		symbol = dlsym(handle, _name.c_str());
		if (symbol == nullptr)
			fmt::print("Failed loading function {}: {}\n", name, dlerror());
	}
	return (symbol);
}

void NativeUnloadObject(void* handle)
{
	if (handle != nullptr)
		dlclose(handle);
}
#endif

SharedCore::SharedCore(std::string_view absFilePath)
{
	handle = NativeLoadObject(absFilePath.data());
	if(handle == nullptr)
		throw std::runtime_error("Could not load core.");
	// Load every function from the shared object into the Ptr functions
#define OCGFUNC(ret, name, args, argnames) \
	do{ \
	void* funcPtr = NativeLoadFunction(handle, #name); \
	(name##_Ptr) = reinterpret_cast<decltype(name##_Ptr)>(funcPtr); \
	if(name##_Ptr == nullptr) \
	{ \
		NativeUnloadObject(handle); \
		throw std::runtime_error("Could not load API function."); \
	} \
	}while(0);
#include "ocgapi_funcs.inl"
#undef OCGFUNC
}

SharedCore::~SharedCore()
{
	NativeUnloadObject(handle);
}

// Forward every call from the interface to the shared object functions
#define OCGFUNC(ret, name, args, argnames) \
	ret SharedCore::name args \
	{ \
		return static_cast<ret>(name##_Ptr argnames); \
	}
#include "ocgapi_funcs.inl"
#undef OCGFUNC

} // namespace Multirole

} // namespace Ignis
