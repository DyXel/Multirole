#include "Replay.hpp"

#include "Constants.hpp"

namespace YGOPro
{

#include "Write.inl"

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

void Replay::RecordMsg(const std::vector<uint8_t>& msg)
{
	// Filter out some useless messages.
	switch(msg[0U])
	{
		case MSG_HINT:
		{
			switch(msg[1U])
			{
				// Do not record player specific hints.
				case 1U: case 2U:
				case 3U: case 5U:
					return;
			}
		}
		case MSG_SELECT_BATTLECMD:
		case MSG_SELECT_IDLECMD:
		case MSG_SELECT_EFFECTYN:
		case MSG_SELECT_YESNO:
		case MSG_SELECT_OPTION:
		case MSG_SELECT_CHAIN:
		case MSG_SELECT_PLACE:
		case MSG_SELECT_DISFIELD:
		case MSG_SELECT_POSITION:
		case MSG_SELECT_COUNTER:
		case MSG_SELECT_SUM:
		case MSG_SORT_CARD:
		case MSG_SORT_CHAIN:
		case MSG_ROCK_PAPER_SCISSORS:
		case MSG_ANNOUNCE_RACE:
		case MSG_ANNOUNCE_ATTRIB:
		case MSG_ANNOUNCE_CARD:
		case MSG_ANNOUNCE_NUMBER:
		case MSG_SELECT_CARD:
		case MSG_SELECT_TRIBUTE:
		case MSG_SELECT_UNSELECT_CARD:
			return;
	}
	messages.emplace_back(msg);
}

void Replay::RecordResponse(const std::vector<uint8_t>& response)
{
	responses.emplace_back(response);
}

void Replay::Serialize()
{

}

// ***** YRPX Binary format *****
// ReplayHeader
// team0Count [uint8_t]
// team0Names [20 uint16_t * team0Count]
// team1Count [uint8_t]
// team1Names [20 uint16_t * team1Count]
// duelFlags [uint32_t]
// DECKS
// Core messages: uint8_t msgType, followed by uint32_t length of message, followed by uint8_t * length
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

} // namespace YGOPro
