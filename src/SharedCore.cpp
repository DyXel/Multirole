#include "SharedCore.hpp"

#include <type_traits>
#include <stdexcept> // std::runtime_error
#include <fmt/printf.h>

namespace Placeholder4
{

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

void* NativeLoadObject(const char* file)
{
// NOTE: We might need a way to identify if the platform is WINRT
#ifdef __WINRT__
	/* WinRT only publically supports LoadPackagedLibrary() for loading .dll
		files.  LoadLibrary() is a private API, and not available for apps
		(that can be published to MS' Windows Store.)
	*/
	void* handle = (void*)LoadPackagedLibrary(file, 0);
#else
	void* handle = (void*)LoadLibrary(file);
#endif
	/* Generate an error message if all loads failed */
	if (handle == nullptr) // TODO: get and print error string
		fmt::print("Failed loading DLL {}\n", file);
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

} // namespace Placeholder4
