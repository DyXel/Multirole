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
	using CardData = OCG_CardData;

	virtual const CardData& DataFromCode(uint32_t code) = 0;
	virtual void DataUsageDone(const CardData& data) = 0;
protected:
	inline ~IDataSupplier() = default;
};

} // namespace Core

} // namespace Multirole

} // namespace Ignis

#endif // IDATASUPPLIER_HPP
