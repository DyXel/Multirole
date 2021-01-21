#ifndef LOBBY_HPP
#define LOBBY_HPP
#include <list>
#include <shared_mutex>
#include <random>
#include <unordered_map>

#include "IRoomManager.hpp"
#include "Room/Instance.hpp"

namespace Ignis::Multirole
{

class Lobby final : public IRoomManager
{
public:
	Lobby();

	std::shared_ptr<Room::Instance> GetRoomById(uint32_t id) const;
	std::size_t GetStartedRoomsCount() const;
	std::list<Room::Instance::Properties> GetAllRoomsProperties() const;

	void CloseNonStartedRooms();
private:
	std::mt19937 rng;
	std::unordered_map<uint32_t, std::shared_ptr<Room::Instance>> rooms;
	mutable std::shared_mutex mRooms;

	// IRoomManager overrides
	std::tuple<uint32_t, uint32_t> Add(std::shared_ptr<Room::Instance> room) override;
	void Remove(uint32_t roomId) override;
};

} // namespace Ignis::Multirole

#endif // LOBBY_HP
