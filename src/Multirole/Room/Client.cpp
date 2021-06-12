#include "Client.hpp"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include "Instance.hpp"
#include "../Lobby.hpp"
#include "../YGOPro/StringUtils.hpp"

namespace Ignis::Multirole::Room
{

Client::Client(
	Lobby& lobby,
	std::shared_ptr<Instance> r,
	boost::asio::ip::tcp::socket socket,
	std::string ip,
	std::string name)
	:
	lobby(lobby),
	room(std::move(r)),
	strand(room->Strand()),
	socket(std::move(socket)),
	ip(std::move(ip)),
	name(std::move(name)),
	connectionLost(false),
	disconnecting(false),
	position(POSITION_SPECTATOR),
	ready(false),
	originalDeck(std::make_unique<YGOPro::Deck>())
{
	lobby.IncrementConnectionCount(this->ip);
}

Client::~Client()
{
	lobby.DecrementConnectionCount(ip);
}

void Client::Start()
{
	auto self(shared_from_this());
	boost::asio::post(strand,
	[this, self]()
	{
		room->Dispatch(Event::Join{*this});
	});
	DoReadHeader();
}

const std::string& Client::Ip() const
{
	return ip;
}

const std::string& Client::Name() const
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

void Client::MarkKicked() const
{
	room->AddKicked(ip);
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
	if(connectionLost || !socket.is_open())
		return;
	std::scoped_lock lock(mOutgoing);
	const bool writeInProgress = !outgoing.empty();
	outgoing.push(msg);
	if(!writeInProgress)
		DoWrite();
}

void Client::Disconnect()
{
	std::scoped_lock lock(mOutgoing);
	if(outgoing.empty())
		Shutdown();
	else
		disconnecting = true;
}

void Client::DoReadHeader()
{
	auto buffer = boost::asio::buffer(incoming.Data(), YGOPro::CTOSMsg::HEADER_LENGTH);
	auto self(shared_from_this());
	boost::asio::async_read(socket, buffer, boost::asio::bind_executor(strand,
	[this, self](boost::system::error_code ec, std::size_t /*unused*/)
	{
		if(!ec && incoming.IsHeaderValid())
		{
			DoReadBody();
		}
		else if(ec != boost::asio::error::operation_aborted)
		{
			connectionLost = true;
			room->Dispatch(Event::ConnectionLost{*this});
		}
	}));
}

void Client::DoReadBody()
{
	auto buffer = boost::asio::buffer(incoming.Body(), incoming.GetLength());
	auto self(shared_from_this());
	boost::asio::async_read(socket, buffer, boost::asio::bind_executor(strand,
	[this, self](boost::system::error_code ec, std::size_t /*unused*/)
	{
		if(!ec)
		{
			// Unlike Endpoint::RoomHosting, we dont want to finish connection
			// if the message is not properly handled. Just ignore it.
			HandleMsg();
			DoReadHeader();
		}
		else if(ec != boost::asio::error::operation_aborted)
		{
			connectionLost = true;
			room->Dispatch(Event::ConnectionLost{*this});
		}
	}));
}

void Client::DoWrite()
{
	auto self(shared_from_this());
	const auto& front = outgoing.front();
	boost::asio::async_write(socket, boost::asio::buffer(front.Data(), front.Length()),
	[this, self](boost::system::error_code ec, std::size_t /*unused*/)
	{
		if(ec)
			return;
		std::scoped_lock lock(mOutgoing);
		outgoing.pop();
		if(!outgoing.empty())
			DoWrite();
		else if(disconnecting)
			Shutdown();
	});
}

void Client::Shutdown()
{
	boost::system::error_code ignore;
	socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
}

void Client::HandleMsg()
{
	switch(incoming.GetType())
	{
	case YGOPro::CTOSMsg::MsgType::RESPONSE:
	{
		std::vector<uint8_t> data(incoming.GetLength());
		std::memcpy(data.data(), incoming.Body(), data.size());
		room->Dispatch(Event::Response{*this, data});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::UPDATE_DECK:
	{
		const uint8_t* ptr = incoming.Body();
		std::vector<uint32_t> main;
		std::vector<uint32_t> side;
		try
		{
			constexpr auto MAX_CARD_COUNT = (YGOPro::CTOSMsg::MSG_MAX_LENGTH -
				(sizeof(uint32_t) * 2U)) / sizeof(uint32_t);
			const auto mainCount = incoming.Read<uint32_t>(ptr);
			const auto sideCount = incoming.Read<uint32_t>(ptr);
			if(mainCount + sideCount > MAX_CARD_COUNT)
				throw std::out_of_range("deck size would exceed message size");
			main.reserve(mainCount);
			side.reserve(sideCount);
			for(uint32_t i = 0U; i < mainCount; i++)
				main.push_back(incoming.Read<uint32_t>(ptr));
			for(uint32_t i = 0U; i < sideCount; i++)
				side.push_back(incoming.Read<uint32_t>(ptr));
		}
		catch(const std::exception& e)
		{
			return;
		}
		room->Dispatch(Event::UpdateDeck{*this, main, side});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::RPS_CHOICE:
	{
		auto p = incoming.GetRPSChoice();
		if(!p)
			return;
		room->Dispatch(Event::ChooseRPS{*this, p->value});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::TURN_CHOICE:
	{
		auto p = incoming.GetTurnChoice();
		if(!p)
			return;
		room->Dispatch(Event::ChooseTurn{*this, p->value != 0U});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::SURRENDER:
	{
		room->Dispatch(Event::Surrender{*this});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::CHAT:
	{
		using namespace YGOPro;
		auto str16 = BufferToUTF16(incoming.Body(), incoming.GetLength());
		auto str = UTF16ToUTF8(str16);
		room->Dispatch(Event::Chat{*this, str});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::TO_DUELIST:
	{
		room->Dispatch(Event::ToDuelist{*this});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::TO_OBSERVER:
	{
		room->Dispatch(Event::ToObserver{*this});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::READY:
	{
		room->Dispatch(Event::Ready{*this, true});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::NOT_READY:
	{
		room->Dispatch(Event::Ready{*this, false});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::TRY_KICK:
	{
		auto p = incoming.GetTryKick();
		if(!p)
			return;
		room->Dispatch(Event::TryKick{*this, p->pos});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::TRY_START:
	{
		room->Dispatch(Event::TryStart{*this});
		break;
	}
	case YGOPro::CTOSMsg::MsgType::REMATCH:
	{
		auto p = incoming.GetRematch();
		if(!p)
			return;
		room->Dispatch(Event::Rematch{*this, static_cast<bool>(p->answer)});
		break;
	}
	default:
		break;
	}
}

} // namespace Ignis::Multirole::Room
