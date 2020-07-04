#ifndef ROOM_STATE_HPP
#define ROOM_STATE_HPP
#include <array>
#include <optional>
#include <set>
#include <variant>

namespace Ignis::Multirole::Room
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
	void* duelPtr;
	std::array<uint8_t, 2U> currentPos;
	Client* replier;
	std::optional<uint32_t> matchKillReason;
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

} // namespace Ignis::Multirole::Room

#endif // ROOM_STATE_HPP
