#include "Deck.hpp"

#include "Banlist.hpp"

namespace YGOPro
{

Deck::Deck(CodeMap&& m, CodeMap&& e, CodeMap&& s, uint32_t err) :
	main(std::move(m)),
	extra(std::move(e)),
	side(std::move(s)),
	error(err)
{}

const CodeMap& Deck::Main() const
{
	return main;
}

const CodeMap& Deck::Extra() const
{
	return extra;
}

const CodeMap& Deck::Side() const
{
	return side;
}

uint32_t Deck::Error() const
{
	return error;
}

} // namespace YGOPro
