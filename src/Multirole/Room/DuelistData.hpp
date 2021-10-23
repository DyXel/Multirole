#ifndef ROOM_DUELISTDATA_HPP
#define ROOM_DUELISTDATA_HPP
#include <array>
#include <cstdint>
#include <utility>

namespace Ignis::Multirole::Room
{

struct DuelistData
{
	// Encoded position. The client interprets that however it sees fit.
	uint8_t pos;
	// uint16_t * 20 == 40 bytes. We round that value up to 64 in case the
	// conversion to UTF-8 makes the string unexpectedly long.
	uint8_t nameLength; // actual size used to create string view.
	static constexpr std::size_t MAX_NAME_LENGTH = 64U;
	std::array<char, MAX_NAME_LENGTH> name;
};

struct DuelistsMap
{
	// Number of duelists the flattened map is actually storing.
	uint8_t usedCount;
	// The number of duelists is hardcoded all over the codebase to 6, so there
	// is no point in storing more than that.
	static constexpr std::size_t MAX_AMOUNT_OF_DUELISTS = 6U;
	std::array<DuelistData, MAX_AMOUNT_OF_DUELISTS> pairs;
};

} // namespace Ignis::Multirole::Room

#endif // ROOM_DUELISTDATA_HPP
