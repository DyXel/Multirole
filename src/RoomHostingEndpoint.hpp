#ifndef ROOMHOSTINGENDPOINT_HPP
#define ROOMHOSTINGENDPOINT_HPP
#include <mutex>
#include <memory>
#include <set>
#include <asio.hpp>

namespace Ignis
{

namespace Multirole {

class Lobby;
struct TmpClient;

class RoomHostingEndpoint
{
public:
	RoomHostingEndpoint(asio::io_context& ioContext, unsigned short port, Lobby& lobby);
	void Stop();
private:
	asio::ip::tcp::acceptor acceptor;
	Lobby& lobby;
	std::set<std::shared_ptr<TmpClient>> tmpClients;
	std::mutex mTmpClients;

	void Add(std::shared_ptr<TmpClient> tc);
	void Remove(std::shared_ptr<TmpClient> tc);

	void DoAccept();
	void DoReadHeader(std::shared_ptr<TmpClient> tc);
	void DoReadBody(std::shared_ptr<TmpClient> tc);

	bool HandleMsg(std::shared_ptr<TmpClient> tc);
};

} // namespace Ignis

} // namespace Multirole

#endif // ROOMHOSTINGENDPOINT_HPP
