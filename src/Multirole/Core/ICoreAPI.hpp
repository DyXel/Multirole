#ifndef ICOREAPI_HPP
#define ICOREAPI_HPP
#include "../../ocgapi_types.h"

namespace Ignis
{

namespace Multirole {

class ICoreAPI
{
public:
#define OCGFUNC(ret, name, args, argnames) virtual ret name args = 0;
#include "../../ocgapi_funcs.inl"
#undef OCGFUNC
	virtual ~ICoreAPI() = default;
};

} // namespace Multirole

} // namespace Ignis

#endif // ICOREAPI_HPP
