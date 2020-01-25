#ifndef MSGCOMMON_HPP
#define MSGCOMMON_HPP
#include <cstdint>

namespace YGOPro
{

struct HostInfo
{
	uint32_t lflist;
	uint8_t rule;
	uint8_t mode;
	uint8_t duel_rule;
	uint8_t no_check_deck;
	uint8_t no_shuffle_deck;
	uint32_t start_lp;
	uint8_t start_hand;
	uint8_t draw_count;
	uint16_t time_limit;
	uint64_t handshake;
	int32_t team1;
	int32_t team2;
	int32_t best_of;
	uint32_t duel_flag;
	int32_t forbiddentypes;
	uint16_t extra_rules;
};

} // namespace YGOPro

#endif // MSGCOMMON_HPP
