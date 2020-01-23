#include "Client.hpp"

#include "IClientManager.hpp"

namespace Ignis
{

namespace Multirole {

Client::Client(IClientManager& owner, asio::ip::tcp::socket soc) :
	owner(owner),
	soc(std::move(soc))
{
	owner.Add(shared_from_this());
}

} // namespace Multirole

} // namespace Ignis
