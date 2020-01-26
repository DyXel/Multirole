#ifndef ROOM_HPP
#define ROOM_HPP
#include <string>
#include <memory>
#include <set>
#include <mutex>

#include "IClientManager.hpp"
#include "MsgCommon.hpp"

namespace Ignis
{

namespace Multirole {

class IRoomManager;

class Room final : public IClientManager, public std::enable_shared_from_this<Room>
{
public:
	struct OptionsData
	{
		std::string name;
		std::string notes;
		std::string pass;
		YGOPro::HostInfo info;
	};
	Room(IRoomManager& owner, OptionsData options);
	OptionsData Options() const;
	void Add(std::shared_ptr<Client> client) override;
	void Remove(std::shared_ptr<Client> client) override;
private:
	IRoomManager& owner;
	OptionsData options;
	std::set<std::shared_ptr<Client>> clients;
	std::mutex mClients;
};

} // namespace Multirole

} // namespace Ignis

#endif // ROOM_HPP
