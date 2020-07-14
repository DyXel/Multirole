#ifndef ROOM_EVENT_HPP
#define ROOM_EVENT_HPP
#include <cstdint>
#include <string_view>
#include <variant>
#include <vector>

namespace Ignis::Multirole::Room
{

class Client;

namespace Event
{

struct Chat
{
	Client& client;
	std::string_view msg;
};

struct ChooseRPS
{
	Client& client;
	uint8_t value;
};

struct ChooseTurn
{
	Client& client;
	bool goingFirst;
};

struct ConnectionLost
{
	Client& client;
};

struct Join
{
	Client& client;
};

struct Ready
{
	Client& client;
	bool value;
};

struct Rematch
{
	Client& client;
	bool answer;
};

struct Response
{
	Client& client;
	const std::vector<uint8_t>& data;
};

struct Surrender
{
	Client& client;
};

struct TimerExpired
{
	uint8_t team;
};

struct ToDuelist
{
	Client& client;
};

struct ToObserver
{
	Client& client;
};

struct TryKick
{
	Client& client;
	uint8_t pos;
};

struct TryStart
{
	Client& client;
};

struct UpdateDeck
{
	Client& client;
	const std::vector<uint32_t>& main;
	const std::vector<uint32_t>& side;
};

} // namespace Event

using EventVariant = std::variant<
	Event::Chat,
	Event::ChooseRPS,
	Event::ChooseTurn,
	Event::ConnectionLost,
	Event::Join,
	Event::Ready,
	Event::Rematch,
	Event::Response,
	Event::Surrender,
	Event::TimerExpired,
	Event::ToDuelist,
	Event::ToObserver,
	Event::TryKick,
	Event::TryStart,
	Event::UpdateDeck>;

} // namespace Ignis::Multirole::Room

#endif // ROOM_EVENT_HPP
