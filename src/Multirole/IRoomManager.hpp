#ifndef IROOMMANAGER_HPP
#define IROOMMANAGER_HPP
#include <cstdint>
#include <memory>

namespace Ignis::Multirole
{

namespace Room
{

class Instance;

} // namespace Room

class IRoomManager
{
	friend Room::Instance;
private:
	virtual uint32_t Add(std::shared_ptr<Room::Instance> room) = 0;
	virtual void Remove(uint32_t roomId) = 0;
};

} // namespace Ignis::Multirole

#endif // IROOMMANAGER_HPP
