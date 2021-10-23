#ifndef LOBBY_HPP
#define LOBBY_HPP
#include <functional>
#include <list>
#include <shared_mutex>
#include <unordered_map>

#include "RNG/SplitMix64.hpp"
#include "Room/Instance.hpp"

namespace Ignis::Multirole
{

class Lobby final
{
public:
	// Queried data about the room used for listing.
	struct RoomProps
	{
		uint32_t id;
		const YGOPro::HostInfo* hostInfo;
		const std::string* notes;
		bool passworded : 1;
		bool started : 1;
		Room::DuelistsMap duelists;
	};

	Lobby(int maxConnections);

	std::shared_ptr<Room::Instance> GetRoomById(uint32_t id) const;

	// Tells if a particular IP has too many connections already.
	bool HasMaxConnections(const std::string& ip) const;

	// Attempts to close all rooms whose state is not Waiting and
	// clears the dictionary of weak references. Also sets a flag so that
	// if any room is made, they are not added to the dictionary.
	// Returns the number of rooms that were **not** closed.
	std::size_t Close();

	// Creates a single room and adds it to the dictionary.
	std::shared_ptr<Room::Instance> MakeRoom(Room::Instance::CreateInfo& info);

	// Removes dead rooms from the dictionary and calls function f for each
	// non-dead room with its properties as argument.
	void CollectRooms(const std::function<void(const RoomProps&)>& f);

	// Change the number of active connections a particular IP has.
	void IncrementConnectionCount(const std::string& ip);
	void DecrementConnectionCount(const std::string& ip);
private:
	const int maxConnections;
	RNG::SplitMix64 rng;
	bool closed;
	std::unordered_map<uint32_t, std::weak_ptr<Room::Instance>> rooms;
	mutable std::shared_mutex mRooms;
	std::unordered_map<std::string, int> connections;
	mutable std::shared_mutex mConnections;
};

} // namespace Ignis::Multirole

#endif // LOBBY_HP
