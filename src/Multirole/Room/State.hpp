#ifndef ROOM_STATE_HPP
#define ROOM_STATE_HPP
#include <array>
#include <optional>
#include <variant>

namespace Ignis::Multirole::Room
{

class Client;

namespace State
{

struct Waiting
{
	Client* host;
};

struct RockPaperScissor
{
	std::array<uint8_t, 2> choices;
};

struct ChoosingTurn
{
	Client* turnChooser;
};

struct Closing
{};

struct Sidedecking
{};

struct Dueling
{
	bool team0GoingFirst;
	void* duelPtr;
	Client* replier;
};

struct Rematching
{};

} // namespace State

using StateVariant = std::variant<
	State::Waiting,
	State::RockPaperScissor,
	State::ChoosingTurn,
	State::Closing,
	State::Sidedecking,
	State::Dueling,
	State::Rematching>;

using StateOpt = std::optional<StateVariant>;

} // namespace Ignis::Multirole::Room

#endif // ROOM_STATE_HPP
