#ifndef LOBBY_HPP
#define LOBBY_HPP
#include <list>
#include <memory>
#include <mutex>
#include <random>
#include <unordered_map>

#include "IRoomManager.hpp"
#include "Room.hpp"

namespace Ignis
{

namespace Multirole
{

class Lobby final : public IRoomManager
{
public:
	Lobby();
	std::shared_ptr<Room> GetRoomById(uint32_t id);
	std::size_t GetStartedRoomsCount();
	std::list<Room::Properties> GetAllRoomsProperties();
	void CloseNonStartedRooms();
private:
	std::mt19937 rd;
	std::unordered_map<uint32_t, std::shared_ptr<Room>> rooms;
	std::mutex mRooms;

	uint32_t Add(std::shared_ptr<Room> room) override;
	void Remove(uint32_t roomId) override;
};

} // namespace Multirole

} // namespace Ignis

#endif // LOBBY_HP