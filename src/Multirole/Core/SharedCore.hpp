#ifndef SHAREDCORE_HPP
#define SHAREDCORE_HPP
#include <string_view>

#include "ICoreAPI.hpp"

namespace Ignis
{

namespace Multirole {

class SharedCore final : public ICoreAPI
{
public:
	SharedCore(std::string_view absFilePath);
	virtual ~SharedCore();

	// Overrides for ICoreAPI
#define OCGFUNC(ret, name, args, argnames) ret name args override;
#include "../../ocgapi_funcs.inl"
#undef OCGFUNC
private:
	void* handle{nullptr};

	// Function pointers from shared object
#define OCGFUNC(ret, name, args, argnames) ret (*name##_Ptr) args{nullptr};
#include "../../ocgapi_funcs.inl"
#undef OCGFUNC
};

} // namespace Multirole

} // namespace Ignis

#endif // SHAREDCORE_HPP
