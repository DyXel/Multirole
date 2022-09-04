#ifndef IWRAPPER_HPP
#define IWRAPPER_HPP
#include <array>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "../../ocgapi_types.h"

namespace Ignis::Multirole::Core
{

class IDataSupplier;
class IScriptSupplier;
class ILogger;

class Exception final : public std::runtime_error
{
public:
	Exception(std::string_view whatArg) : std::runtime_error(whatArg.data())
	{}
};

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
		using SeedType = std::array<uint64_t, 4>;

		IDataSupplier& dataSupplier;
		IScriptSupplier& scriptSupplier;
		ILogger* optLogger;
		SeedType seed;
		uint64_t flags;
		Player team1;
		Player team2;
	};

	virtual std::pair<int, int> Version() = 0;

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
