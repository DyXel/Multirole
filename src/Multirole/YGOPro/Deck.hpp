#ifndef YGOPRO_DECK_HPP
#define YGOPRO_DECK_HPP
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace YGOPro
{

using CodeVector = std::vector<uint32_t>;

enum DeckType : uint32_t
{
	DECK_TYPE_NONE = 0,
	DECK_TYPE_MAIN,
	DECK_TYPE_EXTRA,
	DECK_TYPE_SIDE
};

struct DeckLimits;
class Banlist;

class Deck
{
public:
	Deck(CodeVector&& m, CodeVector&& e, CodeVector&& s, uint32_t err);

	const CodeVector& Main() const;
	const CodeVector& Extra() const;
	const CodeVector& Side() const;
	uint32_t Error() const;

	// Checks
	DeckType Check(const DeckLimits& limits) const;
	uint32_t Check(const Banlist& banlist) const;
	uint32_t MoreThan3() const;
private:
	CodeVector main;
	CodeVector extra;
	CodeVector side;
	uint32_t error;

	std::unordered_map<uint32_t, int> codeCounts;
};

struct DeckLimits
{
	struct Boundary
	{
		std::size_t min, max;
	}main{40, 60}, extra{0, 15}, side{0, 15};
};

} // namespace Multirole

#endif // YGOPRO_DECK_HPP
