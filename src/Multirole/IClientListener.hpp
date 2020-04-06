#ifndef ICLIENTLISTENER_HPP
#define ICLIENTLISTENER_HPP
#include <cstdint>
#include <string_view>

namespace Ignis::Multirole
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
	virtual void OnUpdateDeck(Client& client, const std::vector<uint32_t>& main,
	                          const std::vector<uint32_t>& side) = 0;
	virtual void OnReady(Client& client, bool value) = 0;
	virtual void OnTryKick(Client& client, uint8_t pos) = 0;
	virtual void OnTryStart(Client& client) = 0;
	virtual void OnRPSChoice(Client& client, uint8_t value) = 0;
};

} // namespace Ignis::Multirole

#endif // ICLIENTLISTENER_HPP
