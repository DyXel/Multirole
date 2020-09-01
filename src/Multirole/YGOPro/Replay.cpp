#include "Replay.hpp"

// struct ReplayHeader
// {
// 	uint32_t id; // REPLAY_YRPX(0x58707279) or REPLAY_YRP1(0x31707279)
// 	uint32_t version; // NOTE: Unused atm, should be set to YGOPro::ClientVersion
// 	uint32_t flag; // REPLAY_LUA64 | REPLAY_NEWREPLAY. Optionally REPLAY_COMPRESSED
// 	uint32_t seed; // Unix timestamp for YRPX. Core duel seed for YRP
// 	uint32_t datasize; // Uncompressed size of whatever is after this header
// 	uint32_t hash; // NOTE: unused
// 	uint8_t props[8]; // Used for compression (?)
// };
//
// ReplayHeader
// team0Count [uint8_t]
// team0Names [20 uint16_t * team0Count]
// team1Count [uint8_t]
// team1Names [20 uint16_t * team1Count]
// duelFlags [uint32_t]
// DECKS
// Core messages: uint8_t msgType, followed by uint32_t length of message, followed by uint8_t * length
//
//
// OLD_REPLAY_FORMAT
//
// ReplayHeader
// team0Count [uint8_t]
// team0Names [20 uint16_t * team0Count]
// team1Count [uint8_t]
// team1Names [20 uint16_t * team1Count]
// startingLP [uint32_t]
// startingDrawCount [uint32_t]
// drawCountPerTurn [uint32_t]
// duelFlags [uint32_t]
// DECKS
// EXTRA_CARDS
// Core responses: uint8_t length, followed by uint8_t * length

namespace YGOPro
{

Replay::Replay(uint32_t seed, const HostInfo& info, const CodeVector& extraCards) :
	seed(seed),
	startingLP(info.startingLP),
	startingDrawCount(info.startingDrawCount),
	drawCountPerTurn(info.drawCountPerTurn),
	duelFlags(info.duelFlags),
	extraCards(extraCards)
{}

const std::vector<uint8_t>& Replay::Bytes() const
{
	return bytes;
}

void Replay::AddDuelist(uint8_t team, Duelist&& duelist)
{
	duelists[team].emplace_back(duelist);
}

void Replay::RecordMsg(const STOCMsg& msg)
{
	messages.emplace_back(msg);
}

void Replay::RecordResponse(const std::vector<uint8_t>& response)
{
	responses.emplace_back(response);
}

void Replay::Serialize()
{

}

} // namespace YGOPro
