#include "Deck.hpp"

#include "Banlist.hpp"

namespace YGOPro
{

Deck::Deck(CodeVector&& m, CodeVector&& e, CodeVector&& s, uint32_t err) :
	main(std::move(m)),
	extra(std::move(e)),
	side(std::move(s)),
	error(err)
{
	auto MergeToMap = [&](const CodeVector& cv)
	{
		for(const auto code : cv)
		{
			if(codeCounts.count(code) == 0u)
				codeCounts.emplace(code, 1);
			else
				codeCounts[code]++;
		}
	};
	MergeToMap(main);
	MergeToMap(extra);
	MergeToMap(side);
}

const CodeVector& Deck::Main() const
{
	return main;
}

const CodeVector& Deck::Extra() const
{
	return extra;
}

const CodeVector& Deck::Side() const
{
	return side;
}

uint32_t Deck::Error() const
{
	return error;
}

DeckType Deck::Check(const DeckLimits& limits) const
{
	auto OutOfBound = [](const auto& lim, std::size_t v) -> bool
	{
		return v < lim.min || v > lim.max;
	};
	if(OutOfBound(limits.main, main.size()))
		return DECK_TYPE_MAIN;
	if(OutOfBound(limits.extra, extra.size()))
		return DECK_TYPE_EXTRA;
	if(OutOfBound(limits.side, side.size()))
		return DECK_TYPE_SIDE;
	return DECK_TYPE_NONE;
}

uint32_t Deck::Check(const Banlist& banlist) const
{
	for(const auto& kv : codeCounts)
	{
		uint32_t code = kv.first;
		if(banlist.IsWhitelist() && banlist.Whitelist().count(code) == 0)
			return code;
		if(banlist.Forbidden().count(code) != 0U)
			return code;
		if(kv.second > 1 && (banlist.Limited().count(code) != 0U))
			return code;
		if(kv.second > 2 && (banlist.Semilimited().count(code) != 0U))
			return code;
	}
	return 0;
}

uint32_t Deck::MoreThan3() const
{
	for(const auto& kv : codeCounts)
		if(kv.second > 3)
			return kv.first;
	return 0;
}

} // namespace YGOPro
