#include "Lobby.hpp"

#include <chrono>

namespace Ignis::Multirole
{

std::chrono::time_point<std::chrono::system_clock>::rep TimeNowInt()
{
	return std::chrono::system_clock::now().time_since_epoch().count();
}

// public

Lobby::Lobby() : rng(static_cast<std::mt19937::result_type>(TimeNowInt()))
{}

std::shared_ptr<Room::Instance> Lobby::GetRoomById(uint32_t id) const
{
	std::shared_lock lock(mRooms);
	auto search = rooms.find(id);
	if(search != rooms.end())
		return search->second;
	return nullptr;
}

std::size_t Lobby::GetStartedRoomsCount() const
{
	std::size_t count = 0U;
	std::shared_lock lock(mRooms);
	for(auto& kv : rooms)
		count += static_cast<std::size_t>(kv.second->Started());
	return count;
}

std::list<Room::Instance::Properties> Lobby::GetAllRoomsProperties() const
{
	std::list<Room::Instance::Properties> list;
	std::shared_lock lock(mRooms);
	for(auto& kv : rooms)
		list.emplace_back(kv.second->GetProperties());
	return list;
}

void Lobby::CloseNonStartedRooms()
{
	std::scoped_lock lock(mRooms);
	for(auto& kv : rooms)
		kv.second->TryClose();
}

// private

std::tuple<uint32_t, uint32_t> Lobby::Add(std::shared_ptr<Room::Instance> room)
{
	std::scoped_lock lock(mRooms);
	for(uint32_t newId = 1U; true; newId++)
	{
		if(rooms.count(newId) > 0)
			continue;
		if(rooms.emplace(newId, room).second)
			return {newId, rng()};
	}
}

void Lobby::Remove(uint32_t roomId)
{
	std::scoped_lock lock(mRooms);
	rooms.erase(roomId);
}

} // namespace Ignis::Multirole
