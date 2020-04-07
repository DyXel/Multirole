#include "Deck.hpp"

#include "Banlist.hpp"

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

} // namespace YGOPro
