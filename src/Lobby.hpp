#ifndef LOBBY_HPP
#define LOBBY_HPP
#include <list>
#include <memory>
#include "Room.hpp"

namespace Ignis
{

class Lobby final
{
public:
	using RoomContainerType = std::list<std::weak_ptr<Room>>;
	Lobby();

	const RoomContainerType& GetRooms() const;
private:
	RoomContainerType rooms;
};

} // namespace Ignis

#endif // LOBBY_HP
