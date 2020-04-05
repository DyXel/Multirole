#include "Client.hpp"

#include <asio/bind_executor.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>

#include "IClientListener.hpp"
#include "IClientManager.hpp"
#include "YGOPro/StringUtils.hpp"

namespace Ignis::Multirole
{

Client::Client(IClientListener& listener, IClientManager& owner, asio::io_context::strand& strand, asio::ip::tcp::socket soc, std::string name) :
	listener(listener),
	owner(owner),
	strand(strand),
	soc(std::move(soc)),
	disconnecting(false),
	name(std::move(name)),
	position(POSITION_SPECTATOR),
	ready(false)
{}

void Client::RegisterToOwner()
{
	owner.Add(shared_from_this());
}

void Client::Start()
{
	listener.OnJoin(*this);
	DoReadHeader();
}

std::string Client::Name() const
{
	return name;
}

Client::PosType Client::Position() const
{
	return position;
}

bool Client::Ready() const
{
	return ready;
}

const YGOPro::Deck* Client::Deck() const
{
	return deck.get();
}

void Client::SetPosition(const PosType& p)
{
	position = p;
}

void Client::SetReady(bool r)
{
	ready = r;
}

void Client::SetDeck(std::unique_ptr<YGOPro::Deck>&& newDeck)
{
	deck = std::move(newDeck);
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

void Client::Disconnect()
{
	std::error_code ignoredEc;
	soc.shutdown(asio::ip::tcp::socket::shutdown_both, ignoredEc);
	soc.close(ignoredEc);
	asio::post(strand,
	[this, self = shared_from_this()]()
	{
		owner.Remove(self);
		// NOTE: Destructor of this Client is called here
	});
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

void Client::DoReadHeader()
{
	auto buffer = asio::buffer(incoming.Data(), YGOPro::CTOSMsg::HEADER_LENGTH);
	asio::async_read(soc, buffer, asio::bind_executor(strand,
	[this](const std::error_code& ec, std::size_t /*unused*/)
	{
		if(!ec && incoming.IsHeaderValid())
			DoReadBody();
		else if(ec != asio::error::operation_aborted)
			listener.OnConnectionLost(*this);
	}));
}

void Client::DoReadBody()
{
	auto buffer = asio::buffer(incoming.Body(), incoming.GetLength());
	asio::async_read(soc, buffer, asio::bind_executor(strand,
	[this](const std::error_code& ec, std::size_t /*unused*/)
	{
		if(!ec)
		{
			// Unlike Endpoint::RoomHosting, we dont want to finish connection
			// if the message is not properly handled. Just ignore it.
			HandleMsg();
			DoReadHeader();
		}
		else if(ec != asio::error::operation_aborted)
		{
			listener.OnConnectionLost(*this);
		}
	}));
}

void Client::DoWrite()
{
	const auto& front = outgoing.front();
	asio::async_write(soc, asio::buffer(front.Data(), front.Length()),
	[this](const std::error_code& ec, std::size_t /*unused*/)
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
	switch(incoming.GetType())
	{
	case YGOPro::CTOSMsg::MsgType::CHAT:
	{
		using namespace YGOPro;
		auto str16 = BufferToUTF16(incoming.Body(), incoming.GetLength());
		auto str = UTF16ToUTF8(str16);
		listener.OnChat(*this, str);
		break;
	}
	case YGOPro::CTOSMsg::MsgType::TO_DUELIST:
	{
		listener.OnToDuelist(*this);
		break;
	}
	case YGOPro::CTOSMsg::MsgType::TO_OBSERVER:
	{
		listener.OnToObserver(*this);
		break;
	}
	case YGOPro::CTOSMsg::MsgType::UPDATE_DECK:
	{
		const uint8_t* ptr = incoming.Body();
		std::vector<uint32_t> main, side;
		try
		{
			auto mainCount = incoming.Read<uint32_t>(ptr);
			auto sideCount = incoming.Read<uint32_t>(ptr);
			main.reserve(mainCount);
			side.reserve(sideCount);
			for(decltype(mainCount) i = 0; i < mainCount; i++)
				main.push_back(incoming.Read<uint32_t>(ptr));
			for(decltype(sideCount) i = 0; i < sideCount; i++)
				side.push_back(incoming.Read<uint32_t>(ptr));
		}
		catch(uintptr_t value)
		{}
		listener.OnUpdateDeck(*this, main, side);
		break;
	}
	case YGOPro::CTOSMsg::MsgType::READY:
	{
		listener.OnReady(*this, true);
		break;
	}
	case YGOPro::CTOSMsg::MsgType::NOT_READY:
	{
		listener.OnReady(*this, false);
		break;
	}
	case YGOPro::CTOSMsg::MsgType::TRY_KICK:
	{
		auto p = incoming.GetTryKick();
		if(!p)
			return;
		listener.OnTryKick(*this, p->pos);
		break;
	}
	default:
		break;
	}
}

} // namespace Ignis::Multirole
