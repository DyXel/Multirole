#ifndef YGOPRO_DECK_HPP
#define YGOPRO_DECK_HPP
#include <cstdint>
#include <map>
#include <vector>

namespace YGOPro
{

using CodeMap = std::map<uint32_t, std::size_t>;
using CodeVector = std::vector<uint32_t>;

class Deck final
{
public:
	Deck() noexcept; // Empty deck, valid when dontCheckDeck is true.
	Deck(CodeVector&& m, CodeVector&& e, CodeVector&& s, uint32_t err) noexcept;

	const CodeVector& Main() const noexcept;
	const CodeVector& Extra() const noexcept;
	const CodeVector& Side() const noexcept;
	uint32_t Error() const noexcept;

	// Amalgamate all card codes into a single map.
	CodeMap GetCodeMap() const noexcept;
private:
	CodeVector main;
	CodeVector extra;
	CodeVector side;
	uint32_t error;
};

} // namespace YGOPro

#endif // YGOPRO_DECK_HPP
