#include "Lobby.hpp"

namespace Ignis
{

namespace Multirole
{

// public

Lobby::Lobby()
{}

std::shared_ptr<Room> Lobby::GetRoomById(uint32_t id)
{
	std::lock_guard<std::mutex> lock(mRooms);
	auto search = rooms.find(id);
	if(search != rooms.end())
		return search->second;
	return nullptr;
}

std::size_t Lobby::GetStartedRoomsCount()
{
	std::size_t count = 0u;
	{
		std::lock_guard<std::mutex> lock(mRooms);
		for(auto& kv : rooms)
			count += kv.second->State() != Room::WAITING;
	}
	return count;
}

std::list<Room::Properties> Lobby::GetAllRoomsProperties()
{
	std::list<Room::Properties> list;
	{
		std::lock_guard<std::mutex> lock(mRooms);
		for(auto& kv : rooms)
			list.emplace_back(kv.second->GetProperties());
	}
	return list;
}

void Lobby::StopNonStartedRooms()
{
	std::lock_guard<std::mutex> lock(mRooms);
	for(auto& kv : rooms)
		if(kv.second->State() == Room::WAITING)
			kv.second->Stop();
}

// private

uint32_t Lobby::Add(std::shared_ptr<Room> room)
{
	std::lock_guard<std::mutex> lock(mRooms);
	for(uint32_t newId = 1u; true; newId++)
		if(rooms.count(newId) == 0 && rooms.emplace(newId, room).second)
			return newId;
}

void Lobby::Remove(uint32_t roomId)
{
	std::lock_guard<std::mutex> lock(mRooms);
	rooms.erase(roomId);
}

} // namespace Multirole

} // namespace Ignis
