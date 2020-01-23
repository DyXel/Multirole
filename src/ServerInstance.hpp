#ifndef SERVERINSTANCE_HPP
#define SERVERINSTANCE_HPP
#include <asio.hpp>
#include <nlohmann/json.hpp>

#include "Lobby.hpp"
#include "LobbyListingEndpoint.hpp"
#include "RoomHostingEndpoint.hpp"

namespace Ignis
{

class ServerInstance final
{
public:
	ServerInstance();
	int Run();
private:
	asio::io_context lIoCtx; // Lobby Io Context
	asio::io_context wsIoCtx; // Websocket Io Context
	nlohmann::json cfg;
	Lobby lobby;
	LobbyListingEndpoint lle;
	RoomHostingEndpoint rhe;
	asio::signal_set signalSet;

	void DoWaitSignal();
	void Stop();
};

} // namespace Ignis

#endif // SERVERINSTANCE_HPP
