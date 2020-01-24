#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <asio.hpp>

namespace Ignis
{

namespace Multirole {

class IClientManager;

class Client final : public std::enable_shared_from_this<Client>
{
public:
	struct Properties
	{
		std::string name;
// 		Deck deck;
	};
	Client(IClientManager& owner, const Properties& initial, asio::ip::tcp::socket soc);
private:
	IClientManager& owner;
	Properties properties;
	asio::ip::tcp::socket soc;
};

} // namespace Multirole

} // namespace Ignis

#endif // CLIENT_HPP
