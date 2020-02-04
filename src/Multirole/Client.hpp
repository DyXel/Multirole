#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <utility>
#include <queue>
#include <mutex>

#include <asio/io_context_strand.hpp>
#include <asio/ip/tcp.hpp>

#include "YGOPro/CTOSMsg.hpp"
#include "YGOPro/STOCMsg.hpp"

namespace Ignis
{

namespace Multirole
{

class IClientListener;
class IClientManager;

class Client final : public std::enable_shared_from_this<Client>
{
public:
	using PosType = std::pair<uint8_t, uint8_t>;
	static constexpr PosType POSITION_SPECTATOR = {UINT8_MAX, UINT8_MAX};

	Client(IClientListener& listener, IClientManager& owner, asio::io_context::strand& strand, asio::ip::tcp::socket soc, std::string name);
	std::string Name() const;
	PosType Position() const;
	bool Ready() const;
	void RegisterToOwner();
	void SetPosition(const PosType& p);
	void SetReady(bool r);
	void Start();
	void Disconnect();
	void DeferredDisconnect();
	void Send(const YGOPro::STOCMsg& msg);
private:
	IClientListener& listener;
	IClientManager& owner;
	asio::io_context::strand& strand;
	asio::ip::tcp::socket soc;
	bool disconnecting;
	std::string name;
	PosType position;
	bool ready;
	YGOPro::CTOSMsg incoming;
	std::queue<YGOPro::STOCMsg> outgoing;
	std::mutex mOutgoing;

	void PostUnregisterFromOwner();

	void DoReadHeader();
	void DoReadBody();
	void DoWrite();

	void HandleMsg();
};

} // namespace Multirole

} // namespace Ignis

#endif // CLIENT_HPP
