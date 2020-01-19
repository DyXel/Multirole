#ifndef IDUELAPI_HPP
#define IDUELAPI_HPP
#include "ocgapi_types.hpp"

namespace Placeholder4
{

class IDuelAPI
{
public:
#define OCGFUNC(ret, name, params) virtual ret name params = 0;
#include "ocgapi_funcs.inl"
#undef OCGFUNC
};

} // namespace Placeholder4

#endif // IDUELAPI_HPP
