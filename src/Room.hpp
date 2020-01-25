#ifndef ROOM_HPP
#define ROOM_HPP
#include <string>
#include <memory>
#include <set>
#include <mutex>

#include "IClientManager.hpp"

namespace Ignis
{

namespace Multirole {

class IRoomManager;

class Room final : public IClientManager, public std::enable_shared_from_this<Room>
{
public:
	struct Options
	{
		std::string name;
		std::string notes;
		std::string pass;
		int bestOf;
		int duelFlags;
		int rule;
		int extraRules;
		int forbiddenTypes;
		short startingDrawCount;
		short drawCountPerTurn;
		int startingLP;
		int timeLimitInSeconds;
		int banlistHash;
		bool dontShuffleDeck;
		bool dontCheckDeck;
	};
	Room(IRoomManager& owner, Options initial);
	void Add(std::shared_ptr<Client> client) override;
	void Remove(std::shared_ptr<Client> client) override;
	Options GetOptions() const;
private:
	IRoomManager& owner;
	Options options;
	std::mutex m;
};

} // namespace Multirole

} // namespace Ignis

#endif // ROOM_HPP
