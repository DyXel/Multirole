#ifndef ROOM_HPP
#define ROOM_HPP
#include <string>
#include "ocgapi_types.hpp"

namespace Ignis
{

struct RoomOptions
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

class Room
{
public:
	Room(const RoomOptions& initial);
	RoomOptions GetOptions() const;
private:
	RoomOptions options;
};

} // namespace Ignis

#endif // ROOM_HPP
