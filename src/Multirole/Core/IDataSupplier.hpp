#ifndef IDATASUPPLIER_HPP
#define IDATASUPPLIER_HPP
#include "../../ocgapi_types.h"

namespace Ignis::Multirole::Core
{

class IDataSupplier
{
public:
	using CardData = OCG_CardData;

	virtual const CardData& DataFromCode(uint32_t code) const noexcept = 0;
	virtual void DataUsageDone(const CardData& data) const noexcept = 0;
protected:
	inline ~IDataSupplier() = default;
};

} // namespace Ignis::Multirole::Core

#endif // IDATASUPPLIER_HPP
