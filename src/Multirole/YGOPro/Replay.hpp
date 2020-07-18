#ifndef YGOPRO_REPLAY_HPP
#define YGOPRO_REPLAY_HPP
#include <array>
#include <cstdint>
#include <string>
#include <list>
#include <vector>

#include "Deck.hpp"
#include "MsgCommon.hpp"
#include "STOCMsg.hpp"

namespace YGOPro
{

class Replay
{
public:
	struct Duelist
	{
		std::string name;
		CodeVector main;
		CodeVector side;
	};

	Replay(uint32_t seed, const HostInfo& info, const CodeVector& extraCards);

	void AddDuelist(uint8_t team, Duelist&& duelist);

	void RecordMsg(const STOCMsg& msg);
	void RecordResponse(const std::vector<uint8_t>& response);

	const std::vector<uint8_t>& Flush();
private:
	const uint32_t seed;
	const uint32_t startingLP;
	const uint32_t startingDrawCount;
	const uint32_t drawCountPerTurn;
	const uint32_t duelFlags;
	const CodeVector extraCards;

	std::array<std::list<Duelist>, 2U> duelists;
	std::list<STOCMsg> messages;
	std::list<std::vector<uint8_t>> responses;

	std::optional<std::vector<uint8_t>> bytes;
};

} // namespace YGOPro

#endif // YGOPRO_REPLAY_HPP
