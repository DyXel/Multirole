#ifndef ICLIENTLISTENER_HPP
#define ICLIENTLISTENER_HPP
#include <memory>

namespace Ignis
{

namespace Multirole
{

class Client;

class IClientListener
{
	friend Client;
private:
	virtual void OnJoin(std::shared_ptr<Client> client) = 0;
	virtual void OnConnectionLost(std::shared_ptr<Client> client) = 0;
// 	virtual void OnChat(std::shared_ptr<Client> client, std::string txt) = 0;
};

} // namespace Multirole

} // namespace Ignis

#endif // ICLIENTLISTENER_HPP
