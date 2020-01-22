#ifndef SERVERINSTANCE_HPP
#define SERVERINSTANCE_HPP
#include <asio.hpp>
#include <nlohmann/json.hpp>

#include "Lobby.hpp"
#include "LobbyListEndpoint.hpp"

namespace Ignis
{

class ServerInstance final
{
public:
	ServerInstance();
	int Run();
private:
	asio::io_context lobbyIoContext;
	nlohmann::json cfg;
	Lobby lobby;
	LobbyListEndpoint lle;
	asio::signal_set signalSet;

	void DoWaitSignal();
	void Stop();
	void Terminate();
};

} // namespace Ignis

#endif // SERVERINSTANCE_HPP
