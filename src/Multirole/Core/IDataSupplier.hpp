#ifndef IDATASUPPLIER_HPP
#define IDATASUPPLIER_HPP
#include "../../ocgapi_types.h"

namespace Ignis
{

namespace Multirole
{

namespace Core
{

class IDataSupplier
{
public:
	virtual const OCG_CardData& DataFromCode(uint32_t code) = 0;
	virtual void DataUsageDone(const OCG_CardData& data) = 0;
};

} // namespace Core

} // namespace Multirole

} // namespace Ignis

#endif // IDATASUPPLIER_HPP
