#ifndef SERVERINSTANCE_HPP
#define SERVERINSTANCE_HPP
#include <map>

#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <nlohmann/json.hpp>

#include "BanlistProvider.hpp"
#include "CoreProvider.hpp"
#include "DataProvider.hpp"
#include "GitRepo.hpp"
#include "Lobby.hpp"
#include "ScriptProvider.hpp"
#include "Endpoint/LobbyListing.hpp"
#include "Endpoint/RoomHosting.hpp"

namespace Ignis::Multirole
{

class Instance final
{
public:
	Instance(const nlohmann::json& cfg);
	int Run();
private:
	asio::io_context whIoCtx; // Webhooks Io Context
	asio::io_context lIoCtx; // Lobby Io Context
	asio::executor_work_guard<asio::io_context::executor_type> lIoCtxGuard;
	unsigned int hostingConcurrency;
	DataProvider dataProvider;
	ScriptProvider scriptProvider;
	CoreProvider coreProvider;
	BanlistProvider banlistProvider;
	Lobby lobby;
	Endpoint::LobbyListing lobbyListing;
	Endpoint::RoomHosting roomHosting;
	asio::signal_set signalSet;
	std::map<std::string, GitRepo> repos;

	void DoWaitSignal();
	void Stop();
};

} // namespace Ignis::Multirole

#endif // SERVERINSTANCE_HPP
