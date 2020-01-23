#ifndef ICLIENTMANAGER_HPP
#define ICLIENTMANAGER_HPP
#include <memory>

namespace Ignis
{

namespace Multirole {

class Client;

class IClientManager
{
public:
	virtual void Add(std::shared_ptr<Client> client) = 0;
	virtual void Remove(std::shared_ptr<Client> client) = 0;
};

} // namespace Multirole

} // namespace Ignis

#endif // ICLIENTMANAGER_HPP
