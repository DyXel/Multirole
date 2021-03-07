#ifndef SERVERINSTANCE_HPP
#define SERVERINSTANCE_HPP
#include <map>

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <nlohmann/json.hpp>

#include "GitRepo.hpp"
#include "Lobby.hpp"
#include "Service.hpp"
#include "Endpoint/LobbyListing.hpp"
#include "Endpoint/RoomHosting.hpp"
#include "Service/BanlistProvider.hpp"
#include "Service/CoreProvider.hpp"
#include "Service/DataProvider.hpp"
#include "Service/ReplayManager.hpp"
#include "Service/ScriptProvider.hpp"

namespace Ignis::Multirole
{

class Instance final
{
public:
	Instance(const nlohmann::json& cfg);
	int Run();
private:
	boost::asio::io_context whIoCtx; // Webhooks Io Context
	boost::asio::io_context lIoCtx; // Lobby Io Context
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> lIoCtxGuard;
	unsigned int hostingConcurrency;
	Service::BanlistProvider banlistProvider;
	Service::CoreProvider coreProvider;
	Service::DataProvider dataProvider;
// 	Service::LogHandler logHandler;
	Service::ReplayManager replayManager;
	Service::ScriptProvider scriptProvider;
	Service service;
	Lobby lobby;
	Endpoint::LobbyListing lobbyListing;
	Endpoint::RoomHosting roomHosting;
	boost::asio::signal_set signalSet;
	std::map<std::string, GitRepo> repos;

	void DoWaitSignal();
	void Stop();
};

} // namespace Ignis::Multirole

#endif // SERVERINSTANCE_HPP
