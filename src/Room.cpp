#include "Room.hpp"

#include "IRoomManager.hpp"

namespace Ignis
{

namespace Multirole {

Room::Room(IRoomManager& owner, const Options& initial) : owner(owner), options(initial)
{
	owner.Add(shared_from_this());
}

void Room::Add(std::shared_ptr<Client> client)
{
// 	std::lock_guard<std::mutex> lock(m);
// 	clients.insert(room);
}

void Room::Remove(std::shared_ptr<Client> client)
{
// 	std::lock_guard<std::mutex> lock(m);
// 	clients.erase(room);
}

Room::Options Room::GetOptions() const
{
	return options;
}

} // namespace Multirole

} // namespace Ignis
