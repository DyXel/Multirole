#ifndef YGOPRO_DECK_HPP
#define YGOPRO_DECK_HPP
#include <cstdint>
#include <vector>

namespace YGOPro
{

using CodeVector = std::vector<uint32_t>;

struct DeckLimits
{
	struct Boundary
	{
		std::size_t min, max;
	}main{40, 60}, extra{0, 15}, side{0, 15};
};

class Deck
{
public:
	Deck(CodeVector&& m, CodeVector&& e, CodeVector&& s, uint32_t err);

	const CodeVector& Main() const;
	const CodeVector& Extra() const;
	const CodeVector& Side() const;
	uint32_t Error() const;
private:
	CodeVector main;
	CodeVector extra;
	CodeVector side;
	uint32_t error;
};

} // namespace Multirole

#endif // YGOPRO_DECK_HPP
