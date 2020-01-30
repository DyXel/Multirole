#include "Lobby.hpp"

#include <chrono>

namespace Ignis
{

namespace Multirole
{

std::chrono::time_point<std::chrono::system_clock>::rep TimeNowInt()
{
	return std::chrono::system_clock::now().time_since_epoch().count();
}

// public

Lobby::Lobby() : rd(static_cast<std::mt19937::result_type>(TimeNowInt()))
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

void Lobby::CloseNonStartedRooms()
{
	std::lock_guard<std::mutex> lock(mRooms);
	for(auto& kv : rooms)
		kv.second->TryClose();
}

// private

uint32_t Lobby::Add(std::shared_ptr<Room> room)
{
	std::lock_guard<std::mutex> lock(mRooms);
	for(uint32_t newId = rd(); true; newId = rd())
	{
		if(newId == 0 || rooms.count(newId) > 0)
			continue;
		if(rooms.emplace(newId, room).second)
			return newId;
	}
}

void Lobby::Remove(uint32_t roomId)
{
	std::lock_guard<std::mutex> lock(mRooms);
	rooms.erase(roomId);
}

} // namespace Multirole

} // namespace Ignis
