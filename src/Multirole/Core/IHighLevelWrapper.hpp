#ifndef IHIGHLEVELWRAPPER_HPP
#define IHIGHLEVELWRAPPER_HPP
#include <cstdint>
#include <vector>
#include "../../ocgapi_types.h"

namespace Ignis
{

namespace Multirole
{

namespace Core
{

class IDataSupplier;
class IScriptSupplier;
class ILogger;

class IHighLevelWrapper
{
public:
	using Buffer = std::vector<uint8_t>;
	using Duel = OCG_Duel;

	enum DuelStatus
	{
		DUEL_STATUS_END,
		DUEL_STATUS_WAITING,
		DUEL_STATUS_CONTINUE,
	};

	using Player = OCG_Player;

	struct DuelOptions
	{
		uint32_t seed;
		int flags;
		Player team1;
		Player team2;
	};

	using QueryInfo = OCG_QueryInfo;

	virtual ~IHighLevelWrapper() = default;

	virtual void SetDataSupplier(IDataSupplier* cds) = 0;
	virtual IDataSupplier* GetDataSupplier() = 0;
	virtual void SetScriptSupplier(IScriptSupplier* ss) = 0;
// 	virtual IScriptSupplier* SetScriptSupplier() = 0;
	virtual void SetLogger(ILogger* l) = 0;
// 	virtual ILogger* GetLogger() = 0;

	virtual Duel CreateDuel(const DuelOptions& opts) = 0;
	virtual void DestroyDuel(Duel duel) = 0;
	virtual void AddCard(Duel duel, const OCG_NewCardInfo& info) = 0;
	virtual void Start(Duel duel) = 0;

	virtual DuelStatus Process(Duel duel) = 0;
	virtual Buffer GetMessages(Duel duel) = 0;
	virtual void SetResponse(Duel duel, const Buffer& buffer) = 0;

	virtual std::size_t QueryCount(Duel duel, uint8_t team, uint32_t loc) = 0;
	virtual Buffer Query(Duel duel, const QueryInfo& info) = 0;
	virtual Buffer QueryLocation(Duel duel, const QueryInfo& info) = 0;
	virtual Buffer QueryField(Duel duel) = 0;
};

} // namespace Core

} // namespace Multirole

} // namespace Ignis

#endif // IHIGHLEVELWRAPPER_HPP
