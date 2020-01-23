#ifndef LOBBY_HPP
#define LOBBY_HPP
#include <set>
#include <memory>
#include <mutex>
#include "IRoomManager.hpp"

namespace Ignis
{

namespace Multirole {

class Lobby final : public IRoomManager
{
public:
	using RoomContainerType = std::set<std::shared_ptr<Room>>;
	Lobby();
	void Add(std::shared_ptr<Room> room) override;
	void Remove(std::shared_ptr<Room> room) override;
	std::size_t GetStartedRoomsCount() const;
	const RoomContainerType GetRoomsCopy();
private:
	RoomContainerType rooms;
	std::mutex m;
};

} // namespace Multirole

} // namespace Ignis

#endif // LOBBY_HP
