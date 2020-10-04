#ifndef HORNETWRAPPER_HPP
#define HORNETWRAPPER_HPP
#include "IWrapper.hpp"

namespace Ignis::Multirole::Core
{

class HornetWrapper final : public IWrapper
{
public:
	HornetWrapper();
	~HornetWrapper();

	Duel CreateDuel(const DuelOptions& opts) override;
	void DestroyDuel(Duel duel) override;
	void AddCard(Duel duel, const NewCardInfo& info) override;
	void Start(Duel duel) override;

	DuelStatus Process(Duel duel) override;
	Buffer GetMessages(Duel duel) override;
	void SetResponse(Duel duel, const Buffer& buffer) override;
	int LoadScript(Duel duel, std::string_view name, std::string_view str) override;

	std::size_t QueryCount(Duel duel, uint8_t team, uint32_t loc) override;
	Buffer Query(Duel duel, const QueryInfo& info) override;
	Buffer QueryLocation(Duel duel, const QueryInfo& info) override;
	Buffer QueryField(Duel duel) override;
};

} // namespace Ignis::Multirole::Core

#endif // HORNETWRAPPER_HPP
