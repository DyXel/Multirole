#ifndef YGOPRO_DECK_HPP
#define YGOPRO_DECK_HPP
#include <cstdint>
#include <unordered_map>

namespace YGOPro
{

using CodeMap = std::unordered_map<uint32_t, std::size_t>;

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
	Deck(CodeMap&& m, CodeMap&& e, CodeMap&& s, uint32_t err);

	const CodeMap& Main() const;
	const CodeMap& Extra() const;
	const CodeMap& Side() const;
	uint32_t Error() const;
private:
	CodeMap main;
	CodeMap extra;
	CodeMap side;
	uint32_t error;
};

} // namespace Multirole

#endif // YGOPRO_DECK_HPP
