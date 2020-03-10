#include "Deck.hpp"

#include "Banlist.hpp"

namespace YGOPro
{

Deck::Deck(CodeVector m, CodeVector e, CodeVector s) :
	main(std::move(m)),
	extra(std::move(e)),
	side(std::move(s))
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

bool Deck::Check(const Banlist& banlist) const
{
	for(const auto& kv : codeCounts)
	{
		if(banlist.IsWhitelist() && banlist.Whitelist().count(kv.first) == 0)
			return false;
		if(banlist.Forbidden().count(kv.first) != 0U)
			return false;
		if(kv.second > 1 && (banlist.Limited().count(kv.first) != 0U))
			return false;
		if(kv.second > 2 && (banlist.Semilimited().count(kv.first) != 0U))
			return false;
	}
	return true;
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

} // namespace YGOPro
