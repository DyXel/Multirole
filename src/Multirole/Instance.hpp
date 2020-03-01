#ifndef SERVERINSTANCE_HPP
#define SERVERINSTANCE_HPP
#include <map>
#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <nlohmann/json.hpp>

#include "DataProvider.hpp"
#include "GitRepo.hpp"
#include "Lobby.hpp"
#include "LoggerToStdout.hpp"
#include "ScriptProvider.hpp"
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
	LoggerToStdout logger;
	nlohmann::json cfg;
	DataProvider dataProvider;
	ScriptProvider scriptProvider;
// 	CoreProvider coreProvider;
	Lobby lobby;
	Endpoint::LobbyListing lobbyListing;
	Endpoint::RoomHosting roomHosting;
	asio::signal_set signalSet;
	std::map<std::string, GitRepo> repos;

	void DoWaitSignal();
	void Stop();
};

} // namespace Multirole

} // namespace Ignis

#endif // SERVERINSTANCE_HPP
