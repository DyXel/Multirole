#ifndef IROOMMANAGER_HPP
#define IROOMMANAGER_HPP
#include <cstdint>
#include <memory>

namespace Ignis
{

namespace Multirole {

class Room;

class IRoomManager
{
	friend Room;
private:
	virtual uint32_t Add(std::shared_ptr<Room> room) = 0;
	virtual void Remove(uint32_t roomId) = 0;
};

} // namespace Multirole

} // namespace Ignis

#endif // IROOMMANAGER_HPP
