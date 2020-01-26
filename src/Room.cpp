#include "Room.hpp"

#include "IRoomManager.hpp"

namespace Ignis
{

namespace Multirole
{

Room::Room(IRoomManager& owner, OptionsData options) :
	owner(owner),
	options(std::move(options))
{
	owner.Add(shared_from_this());
}

Room::OptionsData Room::Options() const
{
	return options;
}

void Room::Add(std::shared_ptr<Client> client)
{
	std::lock_guard<std::mutex> lock(mClients);
	clients.insert(client);
	// TODO: actually properly select position of the new client
}

void Room::Remove(std::shared_ptr<Client> client)
{
	std::lock_guard<std::mutex> lock(mClients);
	clients.erase(client);
	if(clients.empty())
		owner.Remove(shared_from_this());
}

} // namespace Multirole

} // namespace Ignis
