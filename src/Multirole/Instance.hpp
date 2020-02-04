#ifndef SERVERINSTANCE_HPP
#define SERVERINSTANCE_HPP
#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <nlohmann/json.hpp>

#include "Lobby.hpp"
#include "Endpoint/LobbyListing.hpp"
#include "Endpoint/RoomHosting.hpp"

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
	asio::io_context whIoCtx; // Webhooks Io Context
	nlohmann::json cfg;
	Lobby lobby;
	Endpoint::LobbyListing lobbyListing;
	Endpoint::RoomHosting roomHosting;
	asio::signal_set signalSet;

	void DoWaitSignal();
	void Stop();
};

} // namespace Multirole

} // namespace Ignis

#endif // SERVERINSTANCE_HPP
