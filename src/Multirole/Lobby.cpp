#include "Lobby.hpp"

#include <chrono>

namespace Ignis::Multirole
{

namespace
{

inline std::chrono::time_point<std::chrono::system_clock>::rep TimeNowInt()
{
	return std::chrono::system_clock::now().time_since_epoch().count();
}

} // namespace

// public

Lobby::Lobby(int maxConnections) :
	maxConnections(maxConnections),
	rng(static_cast<std::mt19937::result_type>(TimeNowInt())),
	closed(false)
{}

std::shared_ptr<Room::Instance> Lobby::GetRoomById(uint32_t id) const
{
	std::shared_lock lock(mRooms);
	auto search = rooms.find(id);
	if(search != rooms.end())
		return search->second.lock();
	return nullptr;
}

bool Lobby::HasMaxConnections(const std::string& ip) const
{
	if(maxConnections < 0)
		return false;
	std::shared_lock lock(mConnections);
	if(const auto search = connections.find(ip); search != connections.end())
		return search->second >= maxConnections;
	return false;
}

std::size_t Lobby::Close()
{
	std::size_t count = 0U;
	std::scoped_lock lock(mRooms);
	for(auto& kv : rooms)
		if(auto room = kv.second.lock(); room)
			count += static_cast<std::size_t>(!room->TryClose());
	rooms.clear();
	closed = true;
	return count;
}

std::shared_ptr<Room::Instance> Lobby::MakeRoom(Room::Instance::CreateInfo& info)
{
	std::scoped_lock lock(mRooms);
	for(uint32_t newId = 1U; true; newId++)
	{
		if(rooms.count(newId) == 0U)
		{
			info.id = newId;
			break;
		}
	}
	info.seed = rng();
	auto room = std::make_shared<Room::Instance>(info);
	if(!closed)
		rooms.emplace(info.id, room);
	return room;
}

void Lobby::CollectRooms(const std::function<void(const RoomProps&)>& f)
{
	RoomProps props{};
	std::scoped_lock lock(mRooms);
	for(auto it = rooms.begin(), last = rooms.end(); it != last;)
	{
		if(auto room = it->second.lock(); room)
		{
			props.id = it->first;
			auto& r = *room;
			props.hostInfo = &r.HostInfo();
			props.notes = &r.Notes();
			props.passworded = r.IsPrivate();
			props.started = r.Started();
			props.duelists = r.DuelistNames();
			f(props);
			++it;
			continue;
		}
		it = rooms.erase(it);
	}
}

void Lobby::IncrementConnectionCount(const std::string& ip)
{
	if(maxConnections < 0)
		return;
	std::scoped_lock lock(mConnections);
	connections[ip]++;
}

void Lobby::DecrementConnectionCount(const std::string& ip)
{
	if(maxConnections < 0)
		return;
	std::scoped_lock lock(mConnections);
	auto search = connections.find(ip);
	assert(search != connections.end());
	search->second--;
	if(search->second == 0)
		connections.erase(search);
}

} // namespace Ignis::Multirole
