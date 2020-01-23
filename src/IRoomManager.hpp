#ifndef IROOMMANAGER_HPP
#define IROOMMANAGER_HPP
#include <memory>

namespace Ignis
{

class Room;

class IRoomManager
{
public:
	virtual void Add(std::shared_ptr<Room> room) = 0;
	virtual void Remove(std::shared_ptr<Room> room) = 0;
};

} // namespace Ignis

#endif // IROOMMANAGER_HPP
