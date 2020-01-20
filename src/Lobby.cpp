#include "Lobby.hpp"

namespace Ignis
{

Lobby::Lobby()
{}

const Lobby::RoomContainerType& Lobby::GetRooms() const
{
	return rooms;
}

} // namespace Ignis
