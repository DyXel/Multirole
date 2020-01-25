#include "Lobby.hpp"

namespace Ignis
{

namespace Multirole {

Lobby::Lobby()
{}

void Lobby::Add(std::shared_ptr<Room> room)
{
	std::lock_guard<std::mutex> lock(mRooms);
	rooms.insert(room);
}

void Lobby::Remove(std::shared_ptr<Room> room)
{
	std::lock_guard<std::mutex> lock(mRooms);
	rooms.erase(room);
}

std::size_t Lobby::GetStartedRoomsCount() const
{
// 	std::lock_guard<std::mutex> lock(m);
	return 0u; // TODO
}

const Lobby::RoomContainerType Lobby::GetRoomsCopy()
{
	std::lock_guard<std::mutex> lock(mRooms);
	return rooms;
}

} // namespace Multirole

} // namespace Ignis
