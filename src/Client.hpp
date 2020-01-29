#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <utility>
#include <queue>
#include <mutex>

#include <asio/io_context_strand.hpp>
#include <asio/ip/tcp.hpp>

#include "CTOSMsg.hpp"
#include "STOCMsg.hpp"

namespace Ignis
{

namespace Multirole
{

class IClientListener;
class IClientManager;

class Client final : public std::enable_shared_from_this<Client>
{
public:
	using PositionType = std::pair<uint8_t, uint8_t>;
	static constexpr PositionType SPECTATOR = {UINT8_MAX, UINT8_MAX};

	Client(IClientListener& listener, IClientManager& owner, asio::io_context::strand& strand, asio::ip::tcp::socket soc, std::string name);
	std::string Name() const;
	PositionType Position() const;
	bool Ready() const;
	void SetPosition(const PositionType& p);
	void SetReady(bool r);
	void Start();
	void Stop();
	void Send(const YGOPro::STOCMsg& msg);
private:
	IClientListener& listener;
	IClientManager& owner;
	asio::io_context::strand& strand;
	asio::ip::tcp::socket soc;
	bool removingSelf;
	std::string name;
	PositionType position;
	bool ready;
	YGOPro::CTOSMsg incoming;
	std::queue<YGOPro::STOCMsg> outgoing;
	std::mutex mOutgoing;

	void PostRemoveSelf();

	void DoReadHeader();
	void DoReadBody();
	void DoWrite();

	void HandleMsg();
};

} // namespace Multirole

} // namespace Ignis

#endif // CLIENT_HPP
