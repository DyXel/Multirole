#include "Room.hpp"

namespace Ignis
{

Room::Room(const Options& initial) : options(initial)
{}

Room::Options Room::GetOptions() const
{
	return options;
}

} // namespace Ignis
