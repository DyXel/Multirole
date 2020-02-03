#ifndef ICLIENTLISTENER_HPP
#define ICLIENTLISTENER_HPP
#include <cstdint>
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
	virtual void OnToDuelist(Client& client) = 0;
	virtual void OnToObserver(Client& client) = 0;
	virtual void OnReady(Client& client, bool value) = 0;
	virtual void OnTryKick(Client& client, uint8_t pos) = 0;
	virtual void OnTryStart(Client& client) = 0;
};

} // namespace Multirole

} // namespace Ignis

#endif // ICLIENTLISTENER_HPP
