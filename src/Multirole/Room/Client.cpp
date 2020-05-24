#include "Client.hpp"

#include <asio/bind_executor.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>

#include "Instance.hpp"
#include "../YGOPro/StringUtils.hpp"

namespace Ignis::Multirole::Room
{

Client::Client(
	Instance& room,
	asio::ip::tcp::socket&& socket,
	std::string&& name)
	:
	room(room),
	strand(room.Strand()),
	socket(std::move(socket)),
	disconnecting(false),
	name(std::move(name)),
	position(POSITION_SPECTATOR),
	ready(false)
{}

void Client::RegisterToOwner()
{
	room.Add(shared_from_this());
}

void Client::Start(std::shared_ptr<Instance>&& ptr)
{
	asio::post(strand,
	[this, self = shared_from_this(), roomPtr = std::move(ptr)]()
	{
		room.Dispatch(Event::Join{{*this}});
	});
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

const YGOPro::Deck* Client::OriginalDeck() const
{
	return originalDeck.get();
}

const YGOPro::Deck* Client::CurrentDeck() const
{
	if(!currentDeck)
		return originalDeck.get();
	return currentDeck.get();
}

void Client::SetPosition(const PosType& p)
{
	position = p;
}

void Client::SetReady(bool r)
{
	ready = r;
}

void Client::SetOriginalDeck(std::unique_ptr<YGOPro::Deck>&& newDeck)
{
	originalDeck = std::move(newDeck);
}

void Client::SetCurrentDeck(std::unique_ptr<YGOPro::Deck>&& newDeck)
{
	currentDeck = std::move(newDeck);
}

void Client::Send(const YGOPro::STOCMsg& msg)
{
	if(!socket.is_open())
		return;
	std::lock_guard<std::mutex> lock(mOutgoing);
	const bool writeInProgress = !outgoing.empty();
	outgoing.push(msg);
	if(!writeInProgress)
		DoWrite();
}

void Client::Disconnect()
{
	std::error_code ignore;
	socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignore);
	socket.close(ignore);
	asio::post(strand,
	[this, self = shared_from_this()]()
	{
		room.Remove(self);
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
	asio::async_read(socket, buffer, asio::bind_executor(strand,
	[this](const std::error_code& ec, std::size_t /*unused*/)
	{
		if(!ec && incoming.IsHeaderValid())
			DoReadBody();
		else if(ec != asio::error::operation_aborted)
			room.Dispatch(Event::ConnectionLost{{*this}});
	}));
}

void Client::DoReadBody()
{
	auto buffer = asio::buffer(incoming.Body(), incoming.GetLength());
	asio::async_read(socket, buffer, asio::bind_executor(strand,
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
			room.Dispatch(Event::ConnectionLost{{*this}});
		}
	}));
}

void Client::DoWrite()
{
	const auto& front = outgoing.front();
	asio::async_write(socket, asio::buffer(front.Data(), front.Length()),
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
		room.Dispatch(Event::Chat{{*this}, str});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::TO_DUELIST:
	{
		room.Dispatch(Event::ToDuelist{{*this}});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::TO_OBSERVER:
	{
		room.Dispatch(Event::ToObserver{{*this}});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::UPDATE_DECK:
	{
		const uint8_t* ptr = incoming.Body();
		std::vector<uint32_t> main;
		std::vector<uint32_t> side;
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
		{
			//Send DECK_INVALID_SIZE to the client
		}
		room.Dispatch(Event::UpdateDeck{{*this}, main, side});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::READY:
	{
		room.Dispatch(Event::Ready{{*this}, true});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::NOT_READY:
	{
		room.Dispatch(Event::Ready{{*this}, false});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::TRY_KICK:
	{
		auto p = incoming.GetTryKick();
		if(!p)
			return;
		room.Dispatch(Event::TryKick{{*this}, p->pos});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::TRY_START:
	{
		room.Dispatch(Event::TryStart{{*this}});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::RPS_CHOICE:
	{
		auto p = incoming.GetRPSChoice();
		if(!p)
			return;
		room.Dispatch(Event::ChooseRPS{{*this}, p->value});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::TURN_CHOICE:
	{
		auto p = incoming.GetTurnChoice();
		if(!p)
			return;
		room.Dispatch(Event::ChooseTurn{{*this}, p->value != 0U});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::RESPONSE:
	{
		std::vector<uint8_t> data(incoming.GetLength());
		std::memcpy(data.data(), incoming.Body(), data.size());
		room.Dispatch(Event::Response{{*this}, data});
		break;
	}
	default:
		break;
	}
}

} // namespace Ignis::Multirole::Room
