#include "Lobby.hpp"

#include "RNG/Xoshiro256.hpp"
#include <chrono>

namespace Ignis::Multirole
{

namespace
{

using Clock = std::chrono::high_resolution_clock;

inline std::chrono::time_point<Clock>::rep TimeNowInt64()
{
	return Clock::now().time_since_epoch().count();
}

} // namespace

// public

Lobby::Lobby(int maxConnections) :
	maxConnections(maxConnections),
	rng(static_cast<RNG::SplitMix64::StateType>(TimeNowInt64())),
	closed(false)
{}

std::shared_ptr<Room::Instance> Lobby::GetRoomById(uint32_t id) const
{
	std::shared_lock lock(mRooms);
	if(id != 0U && id <= rooms.size())
		return rooms[id - 1U].second.lock();
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
	for(auto& slot : rooms)
		if(auto room = slot.second.lock(); room)
			count += static_cast<std::size_t>(!room->TryClose());
	rooms.clear();
	closed = true;
	return count;
}

std::shared_ptr<Room::Instance> Lobby::MakeRoom(Room::Instance::CreateInfo& info)
{
	std::scoped_lock lock(mRooms);
	info.id = [&]()
	{
		uint32_t id = 1U;
		for(const auto& slot : rooms)
		{
			if(!slot.first)
				break;
			id++;
		}
		return id;
	}();
	info.seed = RNG::Xoshiro256StarStar::StateType
	{{
		rng(),
		rng(),
		rng(),
		rng(),
	}};
	auto room = std::make_shared<Room::Instance>(info);
	if(!closed)
	{
		if(info.id <= rooms.size())
		{
			auto& slot = rooms[info.id - 1U];
			slot.first = true;
			slot.second = room;
		}
		else
		{
			rooms.emplace_back(true, room);
		}
	}
	return room;
}

void Lobby::CollectRooms(const std::function<void(const RoomProps&)>& f)
{
	RoomProps props{};
	uint32_t id = 1U;
	std::scoped_lock lock(mRooms);
	for(auto& slot : rooms)
	{
		if(auto room = slot.second.lock(); room)
		{
			props.id = id;
			auto& r = *room;
			props.hostInfo = &r.HostInfo();
			props.notes = &r.Notes();
			props.passworded = r.IsPrivate();
			props.started = r.Started();
			props.duelists = r.DuelistNames();
			f(props);
		}
		else if(slot.first)
		{
			slot.first = false;
			slot.second.reset(); // deallocates memory.
		}
		id++;
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
