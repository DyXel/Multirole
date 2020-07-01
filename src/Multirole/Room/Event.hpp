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

struct Join : public Detail::ClientEvent
{};

struct ConnectionLost : public Detail::ClientEvent
{};

struct Chat : public Detail::ClientEvent
{
	std::string_view msg;
};

struct ToDuelist : public Detail::ClientEvent
{};

struct ToObserver : public Detail::ClientEvent
{};

struct UpdateDeck : public Detail::ClientEvent
{
	const std::vector<uint32_t>& main;
	const std::vector<uint32_t>& side;
};

struct Ready : public Detail::ClientEvent
{
	bool value;
};

struct TryKick : public Detail::ClientEvent
{
	uint8_t pos;
};

struct TryStart : public Detail::ClientEvent
{};

struct ChooseRPS : public Detail::ClientEvent
{
	uint8_t value;
};

struct ChooseTurn : public Detail::ClientEvent
{
	bool goingFirst;
};

struct Response : public Detail::ClientEvent
{
	const std::vector<uint8_t>& data;
};

struct Surrender : public Detail::ClientEvent
{};

struct Rematch : public Detail::ClientEvent
{
	bool answer;
};

} // namespace Event

using EventVariant = std::variant<
	Event::Join,
	Event::ConnectionLost,
	Event::Chat,
	Event::ToDuelist,
	Event::ToObserver,
	Event::UpdateDeck,
	Event::Ready,
	Event::TryKick,
	Event::TryStart,
	Event::ChooseRPS,
	Event::ChooseTurn,
	Event::Response,
	Event::Surrender,
	Event::Rematch>;

} // namespace Ignis::Multirole::Room

#endif // ROOM_EVENT_HPP
