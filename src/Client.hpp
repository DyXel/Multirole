#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <queue>
#include <mutex>

#include <asio/ip/tcp.hpp>

#include "CTOSMsg.hpp"
#include "STOCMsg.hpp"

namespace Ignis
{

namespace Multirole
{

class IClientManager;

class Client final : public std::enable_shared_from_this<Client>
{
public:
	struct Properties
	{
		bool ready;
// 		Deck deck;
	};
	Client(IClientManager& owner, std::string name, asio::ip::tcp::socket soc);
	std::string Name() const;
	void Send(const YGOPro::STOCMsg& msg);
private:
	IClientManager& owner;
	std::string name;
	asio::ip::tcp::socket soc;
	YGOPro::CTOSMsg incoming;
	std::queue<YGOPro::STOCMsg> outgoing;
	std::mutex mOutgoing;

	void DoReadHeader();
	void DoReadBody();
	void DoWrite();

	void HandleMsg();
};

} // namespace Multirole

} // namespace Ignis

#endif // CLIENT_HPP
