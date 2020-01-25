#include "Room.hpp"

#include "IRoomManager.hpp"

namespace Ignis
{

namespace Multirole
{

Room::Room(IRoomManager& owner, Options initial) :
	owner(owner),
	options(std::move(initial))
{
	owner.Add(shared_from_this());
}

void Room::Add(std::shared_ptr<Client> client)
{
	std::lock_guard<std::mutex> lock(m2);
	clients.insert(client);
	// TODO: actually properly select position of the new client
}

void Room::Remove(std::shared_ptr<Client> client)
{
	std::lock_guard<std::mutex> lock(m2);
	clients.erase(client);
	if(clients.empty())
		owner.Remove(shared_from_this());
}

Room::Options Room::GetOptions()
{
	std::lock_guard<std::mutex> lock(m);
	return options;
}

} // namespace Multirole

} // namespace Ignis
