#ifndef YGOPRO_DECK_HPP
#define YGOPRO_DECK_HPP
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace YGOPro
{

using CodeVector = std::vector<uint32_t>;

class Banlist;

class Deck
{
	Deck(CodeVector m, CodeVector e, CodeVector s);

	bool Check(const Banlist& banlist) const;
	const CodeVector& Main() const;
	const CodeVector& Extra() const;
	const CodeVector& Side() const;
private:
	CodeVector main;
	CodeVector extra;
	CodeVector side;

	std::unordered_map<uint32_t, int> codeCounts;
};

} // namespace Multirole

#endif // YGOPRO_DECK_HPP
