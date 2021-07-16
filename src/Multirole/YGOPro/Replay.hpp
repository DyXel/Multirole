#ifndef YGOPRO_REPLAY_HPP
#define YGOPRO_REPLAY_HPP
#include <array>
#include <cstdint>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "Deck.hpp"
#include "MsgCommon.hpp"

namespace YGOPro
{

class Replay final
{
public:
	struct Duelist
	{
		std::string name;
		CodeVector main;
		CodeVector extra;
	};

	Replay(
		uint32_t unixTimestamp,
		uint32_t seed,
		const HostInfo& info,
		const CodeVector& extraCards) noexcept;

	const std::vector<uint8_t>& Bytes() const noexcept;

	void AddDuelist(uint8_t team, uint8_t pos, Duelist&& duelist) noexcept;

	void RecordMsg(const std::vector<uint8_t>& msg) noexcept;
	void RecordResponse(const std::vector<uint8_t>& response) noexcept;

	void PopBackResponse() noexcept;

	void Serialize() noexcept;
private:
	const uint32_t unixTimestamp;
	const uint32_t seed;
	const uint32_t startingLP;
	const uint32_t startingDrawCount;
	const uint32_t drawCountPerTurn;
	const uint64_t duelFlags;
	const CodeVector extraCards;

	std::array<std::map<uint8_t, Duelist>, 2U> duelists;
	std::list<std::vector<uint8_t>> messages;
	std::list<std::vector<uint8_t>> responses;

	std::vector<uint8_t> bytes;
};

} // namespace YGOPro

#endif // YGOPRO_REPLAY_HPP
