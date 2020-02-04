#include "SharedCore.hpp"

#include <stdexcept> // std::runtime_error
#include <type_traits>

#include "../../DLOpen.hpp"

namespace Ignis
{

namespace Multirole
{

SharedCore::SharedCore(std::string_view absFilePath)
{
	handle = DLOpen::LoadObject(absFilePath.data());
	if(handle == nullptr)
		throw std::runtime_error("Could not load core.");
	// Load every function from the shared object into the Ptr functions
#define OCGFUNC(ret, name, args, argnames) \
	do{ \
	void* funcPtr = DLOpen::LoadFunction(handle, #name); \
	(name##_Ptr) = reinterpret_cast<decltype(name##_Ptr)>(funcPtr); \
	if(name##_Ptr == nullptr) \
	{ \
		DLOpen::UnloadObject(handle); \
		throw std::runtime_error("Could not load API function."); \
	} \
	}while(0);
#include "../../ocgapi_funcs.inl"
#undef OCGFUNC
}

SharedCore::~SharedCore()
{
	DLOpen::UnloadObject(handle);
}

// Forward every call from the interface to the shared object functions
#define OCGFUNC(ret, name, args, argnames) \
	ret SharedCore::name args \
	{ \
		return static_cast<ret>(name##_Ptr argnames); \
	}
#include "../../ocgapi_funcs.inl"
#undef OCGFUNC

} // namespace Multirole

} // namespace Ignis
