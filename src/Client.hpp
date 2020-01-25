#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <queue>
#include <mutex>
#include <asio.hpp>

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
		std::string name;
// 		Deck deck;
	};
	Client(IClientManager& owner, Properties initial, asio::ip::tcp::socket soc);

	void Send(const YGOPro::STOCMsg& msg);
private:
	IClientManager& owner;
	Properties properties;
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
