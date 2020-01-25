#include "Client.hpp"

#include "IClientManager.hpp"

namespace Ignis
{

namespace Multirole {

Client::Client(IClientManager& owner, Properties initial, asio::ip::tcp::socket soc) :
	owner(owner),
	properties(std::move(initial)),
	soc(std::move(soc))
{
	owner.Add(shared_from_this());
}

} // namespace Multirole

} // namespace Ignis
