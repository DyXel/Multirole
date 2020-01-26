#include "Client.hpp"

#include "IClientManager.hpp"

namespace Ignis
{

namespace Multirole
{

Client::Client(IClientManager& owner, std::string name, asio::ip::tcp::socket soc) :
	owner(owner),
	name(std::move(name)),
	soc(std::move(soc))
{
	owner.Add(shared_from_this());
	DoReadHeader();
}

std::string Client::Name() const
{
	return name;
}

void Client::Send(const YGOPro::STOCMsg& msg)
{
	std::lock_guard<std::mutex> lock(mOutgoing);
	const bool WriteInProgress = !outgoing.empty();
	outgoing.push(msg);
	if(!WriteInProgress)
		DoWrite();
}

void Client::DoReadHeader()
{
	auto buffer = asio::buffer(incoming.Data(), YGOPro::CTOSMsg::HEADER_LENGTH);
	asio::async_read(soc, buffer,
	[this](const std::error_code& ec, std::size_t)
	{
		if(!ec && incoming.IsHeaderValid())
			DoReadBody();
		else
			owner.Remove(shared_from_this());
	});
}

void Client::DoReadBody()
{
	auto buffer = asio::buffer(incoming.Body(), incoming.GetLength());
	asio::async_read(soc, buffer,
	[this](const std::error_code& ec, std::size_t)
	{
		if(!ec)
		{
			// Unlike RoomHostingEndpoint, we dont want to finish connection
			// if the message is not properly handled. Just ignore it.
			HandleMsg();
			DoReadHeader();
		}
		else
		{
			owner.Remove(shared_from_this());
		}
	});
}

void Client::DoWrite()
{
	const auto& front = outgoing.front();
// 	auto buffer = asio::buffer(front.Data(), front.Length());
	asio::async_write(soc, asio::buffer(front.Data(), front.Length()),
	[this](const std::error_code& ec, std::size_t)
	{
		if (!ec)
		{
			std::lock_guard<std::mutex> lock(mOutgoing);
			outgoing.pop();
			if (!outgoing.empty())
				DoWrite();
		}
		else
		{
			owner.Remove(shared_from_this());
		}
	});
}

void Client::HandleMsg()
{
	// TODO
}

} // namespace Multirole

} // namespace Ignis
