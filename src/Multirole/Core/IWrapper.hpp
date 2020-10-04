#ifndef IWRAPPER_HPP
#define IWRAPPER_HPP
#include <cstdint>
#include <vector>
#include <string_view>
#include "../../ocgapi_types.h"

namespace Ignis::Multirole::Core
{

class IDataSupplier;
class IScriptSupplier;
class ILogger;

class IWrapper
{
public:
	using Buffer = std::vector<uint8_t>;
	using Duel = OCG_Duel;
	using NewCardInfo = OCG_NewCardInfo;
	using Player = OCG_Player;
	using QueryInfo = OCG_QueryInfo;

	enum class DuelStatus
	{
		DUEL_STATUS_END,
		DUEL_STATUS_WAITING,
		DUEL_STATUS_CONTINUE,
	};

	struct DuelOptions
	{
		IDataSupplier& dataSupplier;
		IScriptSupplier& scriptSupplier;
		ILogger* optLogger;
		uint32_t seed;
		int flags;
		Player team1;
		Player team2;
	};

	virtual Duel CreateDuel(const DuelOptions& opts) = 0;
	virtual void DestroyDuel(Duel duel) = 0;
	virtual void AddCard(Duel duel, const NewCardInfo& info) = 0;
	virtual void Start(Duel duel) = 0;

	virtual DuelStatus Process(Duel duel) = 0;
	virtual Buffer GetMessages(Duel duel) = 0;
	virtual void SetResponse(Duel duel, const Buffer& buffer) = 0;
	virtual int LoadScript(Duel duel, std::string_view name, std::string_view str) = 0;

	virtual std::size_t QueryCount(Duel duel, uint8_t team, uint32_t loc) = 0;
	virtual Buffer Query(Duel duel, const QueryInfo& info) = 0;
	virtual Buffer QueryLocation(Duel duel, const QueryInfo& info) = 0;
	virtual Buffer QueryField(Duel duel) = 0;
protected:
	inline ~IWrapper() = default;
};

} // namespace Ignis::Multirole::Core

#endif // IWRAPPER_HPP
