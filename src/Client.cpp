#include "Client.hpp"

#include <asio/bind_executor.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>

#include "IClientListener.hpp"
#include "IClientManager.hpp"

namespace Ignis
{

namespace Multirole
{

Client::Client(IClientListener& listener, IClientManager& owner, asio::io_context::strand& strand, asio::ip::tcp::socket soc, std::string name) :
	listener(listener),
	owner(owner),
	strand(strand),
	soc(std::move(soc)),
	removingSelf(false),
	name(std::move(name)),
	position(SPECTATOR),
	ready(false)
{}

std::string Client::Name() const
{
	return name;
}

Client::PositionType Client::Position() const
{
	return position;
}

bool Client::Ready() const
{
	return ready;
}

void Client::SetPosition(const PositionType& p)
{
	position = p;
}

void Client::SetReady(bool r)
{
	ready = r;
}

void Client::Start()
{
	owner.Add(shared_from_this());
	DoReadHeader();
}

void Client::Stop()
{
	soc.cancel();
}

void Client::Send(const YGOPro::STOCMsg& msg)
{
	std::lock_guard<std::mutex> lock(mOutgoing);
	const bool WriteInProgress = !outgoing.empty();
	outgoing.push(msg);
	if(!WriteInProgress)
		DoWrite();
}

void Client::PostRemoveSelf()
{
	if(removingSelf)
		return;
	removingSelf = true;
	asio::post(strand,
	[this, self = shared_from_this()]()
	{
		owner.Remove(self);
	});
}

void Client::DoReadHeader()
{
	auto buffer = asio::buffer(incoming.Data(), YGOPro::CTOSMsg::HEADER_LENGTH);
	asio::async_read(soc, buffer, asio::bind_executor(strand,
	[this](const std::error_code& ec, std::size_t)
	{
		if(!ec && incoming.IsHeaderValid())
			DoReadBody();
		else
			PostRemoveSelf();
	}));
}

void Client::DoReadBody()
{
	auto buffer = asio::buffer(incoming.Body(), incoming.GetLength());
	asio::async_read(soc, buffer, asio::bind_executor(strand,
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
			PostRemoveSelf();
		}
	}));
}

void Client::DoWrite()
{
	const auto& front = outgoing.front();
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
			PostRemoveSelf();
		}
	});
}

void Client::HandleMsg()
{
	// TODO
}

} // namespace Multirole

} // namespace Ignis
