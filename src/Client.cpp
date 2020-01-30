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
	disconnecting(false),
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

void Client::RegisterToOwner()
{
	owner.Add(shared_from_this());
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
	listener.OnJoin(shared_from_this());
	DoReadHeader();
}

void Client::Disconnect()
{
	std::error_code ignoredEc;
	soc.shutdown(asio::ip::tcp::socket::shutdown_both, ignoredEc);
	soc.close(ignoredEc);
	PostUnregisterFromOwner();
}

void Client::DeferredDisconnect()
{
	{
		std::lock_guard<std::mutex> lock(mOutgoing);
		if(outgoing.empty())
		{
			Disconnect();
			return;
		}
	}
	disconnecting = true;
}

void Client::Send(const YGOPro::STOCMsg& msg)
{
	if(!soc.is_open())
		return;
	std::lock_guard<std::mutex> lock(mOutgoing);
	const bool writeInProgress = !outgoing.empty();
	outgoing.push(msg);
	if(!writeInProgress)
		DoWrite();
}

void Client::PostUnregisterFromOwner()
{
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
		else if(ec != asio::error::operation_aborted)
			listener.OnConnectionLost(shared_from_this());
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
		else if(ec != asio::error::operation_aborted)
		{
			listener.OnConnectionLost(shared_from_this());
		}
	}));
}

void Client::DoWrite()
{
	const auto& front = outgoing.front();
	asio::async_write(soc, asio::buffer(front.Data(), front.Length()),
	[this](const std::error_code& ec, std::size_t)
	{
		if(ec)
			return;
		std::lock_guard<std::mutex> lock(mOutgoing);
		outgoing.pop();
		if (!outgoing.empty())
			DoWrite();
		else if(disconnecting)
			Disconnect();
	});
}

void Client::HandleMsg()
{
	// TODO
}

} // namespace Multirole

} // namespace Ignis
