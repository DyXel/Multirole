#ifndef LOBBY_HPP
#define LOBBY_HPP
#include <list>
#include <mutex>
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
	std::shared_ptr<Room::Instance> GetRoomById(uint32_t id);
	std::size_t GetStartedRoomsCount();
	std::list<Room::Instance::Properties> GetAllRoomsProperties();
	void CloseNonStartedRooms();
private:
	std::mt19937 rd;
	std::unordered_map<uint32_t, std::shared_ptr<Room::Instance>> rooms;
	std::mutex mRooms;

	// IRoomManager overrides
	uint32_t Add(std::shared_ptr<Room::Instance> room) override;
	void Remove(uint32_t roomId) override;
};

} // namespace Ignis::Multirole

#endif // LOBBY_HP
