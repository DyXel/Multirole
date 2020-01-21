#ifndef ROOM_HPP
#define ROOM_HPP
#include <string>
#include "ocgapi_types.h"

namespace Ignis
{

class Room final
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
	Room(const Options& initial);
	Options GetOptions() const;
private:
	Options options;
};

} // namespace Ignis

#endif // ROOM_HPP
