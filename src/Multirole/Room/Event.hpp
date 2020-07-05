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

namespace Detail
{

struct ClientEvent
{
	Client& client;
};

} // namespace Detail

struct Chat : public Detail::ClientEvent
{
	std::string_view msg;
};

struct ChooseRPS : public Detail::ClientEvent
{
	uint8_t value;
};

struct ChooseTurn : public Detail::ClientEvent
{
	bool goingFirst;
};

struct ConnectionLost : public Detail::ClientEvent
{};

struct Join : public Detail::ClientEvent
{};

struct Ready : public Detail::ClientEvent
{
	bool value;
};

struct Rematch : public Detail::ClientEvent
{
	bool answer;
};

struct Response : public Detail::ClientEvent
{
	const std::vector<uint8_t>& data;
};

struct Surrender : public Detail::ClientEvent
{};

struct TimerExpired
{};

struct ToDuelist : public Detail::ClientEvent
{};

struct ToObserver : public Detail::ClientEvent
{};

struct TryKick : public Detail::ClientEvent
{
	uint8_t pos;
};

struct TryStart : public Detail::ClientEvent
{};

struct UpdateDeck : public Detail::ClientEvent
{
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
