#include "Deck.hpp"

namespace YGOPro
{

Deck::Deck(CodeVector&& m, CodeVector&& e, CodeVector&& s, uint32_t err) :
	main(std::move(m)),
	extra(std::move(e)),
	side(std::move(s)),
	error(err)
{}

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

CodeMap Deck::GetCodeMap() const
{
	CodeMap map;
	auto AddToMap = [&map](const CodeVector& from)
	{
		for(auto code : from)
			map[code]++;
	};
	AddToMap(main);
	AddToMap(extra);
	AddToMap(side);
	return map;
}

} // namespace YGOPro
