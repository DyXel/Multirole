#ifndef ROOM_STATE_HPP
#define ROOM_STATE_HPP
#include <array>
#include <chrono>
#include <deque>
#include <optional>
#include <set>
#include <variant>

#include "../YGOPro/Replay.hpp"
#include "../YGOPro/STOCMsg.hpp"

namespace Ignis::Multirole
{

namespace Core
{

class IWrapper;

} // namespace Core

namespace Room
{

class Client;

namespace State
{

struct ChoosingTurn
{
	Client* turnChooser;
};

struct Closing
{};

struct Dueling
{
	std::shared_ptr<Core::IWrapper> core;
	void* duelPtr;
	uint64_t replayId;
	std::unique_ptr<YGOPro::Replay> replay;
	std::array<uint8_t, 2U> currentPos;
	std::array<uint8_t, 2U> retryCount;
	std::vector<uint8_t> lastHint;
	std::vector<uint8_t> lastRequest;
	Client* replier;
	std::optional<uint32_t> matchKillReason;
	std::deque<YGOPro::STOCMsg> spectatorCache;
	std::array<std::chrono::milliseconds, 2U> timeRemaining;
};

struct Rematching
{
	Client* turnChooser;
	std::set<Client*> answered;
};

struct RockPaperScissor
{
	std::array<uint8_t, 2U> choices;
};

struct Sidedecking
{
	Client* turnChooser;
	std::set<Client*> sidedecked;
};

struct Waiting
{
	Client* host;
};

} // namespace State

using StateVariant = std::variant<
	State::ChoosingTurn,
	State::Closing,
	State::Dueling,
	State::Rematching,
	State::RockPaperScissor,
	State::Sidedecking,
	State::Waiting>;

using StateOpt = std::optional<StateVariant>;

} // namespace Room

} // namespace Ignis::Multirole

#endif // ROOM_STATE_HPP
