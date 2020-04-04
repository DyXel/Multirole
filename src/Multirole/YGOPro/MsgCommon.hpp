#ifndef MSGCOMMON_HPP
#define MSGCOMMON_HPP
#include <cstdint>

namespace YGOPro
{

enum AllowedCards
{
	ALLOWED_CARDS_OCG_ONLY,
	ALLOWED_CARDS_TCG_ONLY,
	ALLOWED_CARDS_OCG_TCG,
	ALLOWED_CARDS_WITH_PRERELEASE,
	ALLOWED_CARDS_ANY
};

struct HostInfo
{
	uint32_t banlistHash;
	uint8_t allowed; // OCG/TCG, etc
	uint8_t mode; // NOTE: UNUSED
	uint8_t duelRule; // NOTE: UNUSED
	uint8_t dontCheckDeck;
	uint8_t dontShuffleDeck;
	uint32_t startingLP;
	uint8_t startingDrawCount;
	uint8_t drawCountPerTurn;
	uint16_t timeLimitInSeconds;
	uint64_t serverHandshake;
	int32_t t1Count;
	int32_t t2Count;
	int32_t bestOf;
	uint32_t duelFlags;
	int32_t forb; // Forbidden types
	uint16_t extraRules; // Sealed Duel, Destiny Draw, etc
};

} // namespace YGOPro

#endif // MSGCOMMON_HPP
