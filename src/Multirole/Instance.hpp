#ifndef SERVERINSTANCE_HPP
#define SERVERINSTANCE_HPP
#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <nlohmann/json.hpp>

#include "Lobby.hpp"
#include "Endpoint/LobbyListingEndpoint.hpp"
#include "Endpoint/RoomHostingEndpoint.hpp"

namespace Ignis
{

namespace Multirole
{

class Instance final
{
public:
	Instance();
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

} // namespace Multirole

} // namespace Ignis

#endif // SERVERINSTANCE_HPP
