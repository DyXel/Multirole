#ifndef ROOMHOSTINGENDPOINT_HPP
#define ROOMHOSTINGENDPOINT_HPP
#include <mutex>
#include <memory>
#include <set>

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

namespace Ignis::Multirole
{

class BanlistProvider;
class CoreProvider;
class DataProvider;
class ScriptProvider;
class Lobby;

namespace Endpoint
{

struct TmpClient;

class RoomHosting
{
public:
	// Data passed on the ctor.
	struct CreateInfo
	{
		asio::io_context& ioCtx;
		unsigned short port;
		BanlistProvider& banlistProvider;
		CoreProvider& coreProvider;
		DataProvider& dataProvider;
		ScriptProvider& scriptProvider;
		Lobby& lobby;
	};

	RoomHosting(CreateInfo&& info);
	void Stop();
private:
	asio::io_context& ioCtx;
	asio::ip::tcp::acceptor acceptor;
	BanlistProvider& banlistProvider;
	CoreProvider& coreProvider;
	DataProvider& dataProvider;
	ScriptProvider& scriptProvider;
	Lobby& lobby;

	std::set<std::shared_ptr<TmpClient>> tmpClients;
	std::mutex mTmpClients;

	void Add(const std::shared_ptr<TmpClient>& tc);
	void Remove(const std::shared_ptr<TmpClient>& tc);

	void DoAccept();
	void DoReadHeader(const std::shared_ptr<TmpClient>& tc);
	void DoReadBody(const std::shared_ptr<TmpClient>& tc);

	bool HandleMsg(const std::shared_ptr<TmpClient>& tc);
};

} // namespace Endpoint

} // namespace Ignis::Multirole

#endif // ROOMHOSTINGENDPOINT_HPP
