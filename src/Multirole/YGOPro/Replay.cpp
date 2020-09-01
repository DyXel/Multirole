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
