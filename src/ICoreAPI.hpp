#ifndef ICOREAPI_HPP
#define ICOREAPI_HPP
#include "ocgapi_types.hpp"

namespace Placeholder4
{

class ICoreAPI
{
public:
#define OCGFUNC(ret, name, params) virtual ret name params = 0;
#include "ocgapi_funcs.inl"
#undef OCGFUNC
};

} // namespace Placeholder4

#endif // ICOREAPI_HPP
