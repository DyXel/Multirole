#ifndef ICLIENTLISTENER_HPP
#define ICLIENTLISTENER_HPP
#include <string_view>

namespace Ignis
{

namespace Multirole
{

class Client;

class IClientListener
{
	friend Client;
private:
	virtual void OnJoin(Client& client) = 0;
	virtual void OnConnectionLost(Client& client) = 0;
	virtual void OnChat(Client& client, std::string_view str) = 0;
};

} // namespace Multirole

} // namespace Ignis

#endif // ICLIENTLISTENER_HPP
