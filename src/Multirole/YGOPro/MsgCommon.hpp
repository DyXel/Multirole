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
	ALLOWED_CARDS_ANY,
};

enum ExtraRule
{
	EXTRA_RULE_SEALED_DUEL         = 0x1,
	EXTRA_RULE_BOOSTER_DUEL        = 0x2,
	EXTRA_RULE_DESTINY_DRAW        = 0x4,
	EXTRA_RULE_CONCENTRATION_DUEL  = 0x8,
	EXTRA_RULE_BOSS_DUEL           = 0x10,
	EXTRA_RULE_BATTLE_CITY         = 0x20,
	EXTRA_RULE_DUELIST_KINGDOM     = 0x40,
	EXTRA_RULE_DIMENSION_DUEL      = 0x80,
	EXTRA_RULE_TURBO_DUEL          = 0x100,
	EXTRA_RULE_DOUBLE_DECK         = 0x200,
	EXTRA_RULE_COMMAND_DUEL        = 0x400,
	EXTRA_RULE_DECK_MASTER         = 0x800,
	EXTRA_RULE_ACTION_DUEL         = 0x1000,
	EXTRA_RULE_DECK_LIMIT_20       = 0x2000,
};

struct ClientVersion
{
	struct
	{
		uint8_t major;
		uint8_t minor;
	} client, core;
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
	uint32_t duelFlagsHigh; // OR'd with duelFlagsLow
	uint32_t handshake;
	ClientVersion version;
	int32_t t0Count;
	int32_t t1Count;
	int32_t bestOf;
	uint32_t duelFlagsLow; // OR'd with duelFlagsHigh
	int32_t forb; // Forbidden types
	uint16_t extraRules; // Double deck, Speed duel, etc

	static constexpr uint64_t OrDuelFlags(uint32_t high, uint32_t low) noexcept
	{
		return low | (static_cast<uint64_t>(high) << 32U);
	}
};

} // namespace YGOPro

#endif // MSGCOMMON_HPP
