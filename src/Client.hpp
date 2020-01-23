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
	Client(IClientManager& owner, asio::ip::tcp::socket soc);
private:
	IClientManager& owner;
	asio::ip::tcp::socket soc;
};

} // namespace Multirole

} // namespace Ignis

#endif // CLIENT_HPP
