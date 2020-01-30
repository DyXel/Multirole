#include "Room.hpp"

#include <fmt/format.h>

#include "Client.hpp"
#include "IRoomManager.hpp"
#include "STOCMsg.hpp"
#include "StringUtils.hpp"

namespace Ignis
{

namespace Multirole
{

enum PlayerChange : uint8_t
{
	SPECTATE   = 0x8,
	READY      = 0x9,
	NOT_READY  = 0xA,
	LEAVE      = 0xB,
};

// Creates a position in the format that the client expects
constexpr uint8_t MakeClientPos(Client::PositionType pos, uint8_t t1max)
{
	return (pos.first * t1max) + pos.second;
}

// Creates a status in the format that the client expects OR'd with pos
constexpr uint8_t MakeClientStatus(uint8_t pos, PlayerChange status)
{
	return (pos << 4) | static_cast<uint8_t>(status);
}

// Creates a host change in the format that the client expects OR'd with pos
constexpr uint8_t MakeClientHost(uint8_t pos, bool isHost)
{
	return (static_cast<uint8_t>(isHost) << 4) | pos;
}

inline YGOPro::STOCMsg MakeChat(uint16_t posOrType, std::string_view str)
{
	using namespace StringUtils;
	YGOPro::STOCMsg::Chat proto{};
	proto.posOrType = posOrType;
	const auto size = UTF16ToBuffer(proto.msg, UTF8ToUTF16(str));
	return YGOPro::STOCMsg(proto).Shrink(size + sizeof(uint16_t));
}

inline  YGOPro::STOCMsg MakeHsPlayerChangeReady(uint8_t pos, bool r)
{
	using YGOPro::STOCMsg;
	STOCMsg::HsPlayerChange proto{};
	proto.status = MakeClientStatus(pos, r ? READY : NOT_READY);
	return STOCMsg(proto);
}

inline YGOPro::STOCMsg MakeHsPlayerChange(uint8_t pos, PlayerChange pc)
{
	using YGOPro::STOCMsg;
	return STOCMsg(STOCMsg::HsPlayerChange{MakeClientStatus(pos, pc)});
}

inline YGOPro::STOCMsg MakeHsWatchChange(std::size_t count)
{
	return {YGOPro::STOCMsg::HsWatchChange{static_cast<uint16_t>(count)}};
}

// public

Room::Room(IRoomManager& owner, asio::io_context& ioCtx, Options options) :
	owner(owner),
	strand(ioCtx),
	options(std::move(options)),
	state(WAITING),
	host(nullptr)
{}

Room::StateEnum Room::State() const
{
	return state;
}

bool Room::CheckPassword(std::string_view str) const
{
	return options.pass == "" || options.pass == str;
}

void Room::RegisterToOwner()
{
	options.id = owner.Add(shared_from_this());
}

Room::Properties Room::GetProperties()
{
	Properties prop;
	prop.info = options.info;
	prop.notes = options.notes;
	prop.passworded = options.pass != "";
	prop.id = options.id;
	prop.state = state;
	{
		std::lock_guard<std::mutex> lock(mDuelists);
		for(auto& kv : duelists)
		{
			auto pos = MakeClientPos(kv.first, options.info.t1Count);
			prop.duelists.emplace(pos, kv.second->Name());
		}
	}
	return prop;
}

asio::io_context::strand& Room::Strand()
{
	return strand;
}

void Room::TryClose()
{
	if(state != WAITING)
		return;
	// TODO: stop timers too
	std::lock_guard<std::mutex> lock(mClients);
	std::lock_guard<std::mutex> lock2(mDuelists);
	duelists.clear();
	for(auto& c : clients)
		c->Disconnect();
}

// private

void Room::OnJoin(Client& client)
{
	switch(state)
	{
	case WAITING:
	{
		// Join to waiting room
		JoinToWaiting(client);
		return;
	}
	default:
	{
		// Join to started duel
		JoinToDuel(client);
		return;
	}
	}
}

void Room::OnConnectionLost(Client& client)
{
	switch(state)
	{
	case WAITING:
	{
		if(host == &client)
		{
			{
				std::lock_guard<std::mutex> lock(mDuelists);
				duelists.clear();
			}
			std::lock_guard<std::mutex> lock(mClients);
			for(auto& c : clients)
				c->Disconnect();
			return;
		}
		client.Disconnect();
		const auto posKey = client.Position();
		if(posKey != Client::POSITION_SPECTATOR)
		{
			uint8_t pos = MakeClientPos(posKey, options.info.t1Count);
			SendToAll(MakeHsPlayerChange(pos, LEAVE));
			std::lock_guard<std::mutex> lock(mDuelists);
			duelists.erase(posKey);
		}
		else
		{
			SendToAll(MakeHsWatchChange(clients.size() - duelists.size() - 1u));
		}
		return;
	}
	default: // Room is currently dueling
	{
		// TODO
		return;
	}
	}
}

void Room::OnChat(Client& client, std::string_view str)
{
	const auto posKey = client.Position();
	if(posKey != Client::POSITION_SPECTATOR)
	{
		SendToAll(MakeChat(MakeClientPos(posKey, options.info.t1Count), str));
	}
	else
	{
		const auto fmtStr = fmt::format("{}: {}", client.Name(), str);
		SendToAll(MakeChat(10u, fmtStr));
	}
}

void Room::Add(std::shared_ptr<Client> client)
{
	std::lock_guard<std::mutex> lock(mClients);
	clients.insert(client);
}

void Room::Remove(std::shared_ptr<Client> client)
{
	std::lock_guard<std::mutex> lock(mClients);
	clients.erase(client);
	if(clients.empty())
		PostUnregisterFromOwner();
}

void Room::JoinToWaiting(Client& client)
{
	if(host == nullptr)
		host = &client;
	using YGOPro::STOCMsg;
	auto TryEmplaceDuelist = [&](Client& c, uint8_t m1, uint8_t m2)
	{
		Client::PositionType p{0u, 0u};
		for(; p.second < m1; p.second++)
		{
			if(duelists.count(p) == 0 && duelists.emplace(p, &c).second)
			{
				c.SetPosition(p);
				return true;
			}
		}
		p.first = 1u;
		for(p.second = 0u; p.second < m2; p.second++)
		{
			if(duelists.count(p) == 0 && duelists.emplace(p, &c).second)
			{
				c.SetPosition(p);
				return true;
			}
		}
		return false;
	};
	auto MakeHsPlayerEnter = [](Client& c, uint8_t m1)
	{
		STOCMsg::HsPlayerEnter proto{};
		using namespace StringUtils;
		UTF16ToBuffer(proto.name, UTF8ToUTF16(c.Name()));
		proto.pos = MakeClientPos(c.Position(), m1);
		return STOCMsg(proto);
	};
	client.Send(STOCMsg::JoinGame{options.info}); // TODO: create on ctor?
	std::lock_guard<std::mutex> lock(mDuelists);
	const uint8_t t1max = options.info.t1Count;
	if(TryEmplaceDuelist(client, t1max, options.info.t2Count))
	{
		// Inform new duelist if they are the host or not
		uint8_t pos = MakeClientPos(client.Position(), t1max);
		client.Send(STOCMsg::TypeChange{MakeClientHost(pos, host == &client)});
		// Send information about new duelist to all clients
		SendToAll(MakeHsPlayerEnter(client, t1max));
		SendToAll(MakeHsPlayerChangeReady(pos, client.Ready()));
		// Send spectator count to new duelist
		client.Send(MakeHsWatchChange(clients.size() - duelists.size()));
	}
	else
	{
		// Send spectator count to everyone
		SendToAll(MakeHsWatchChange(clients.size() - duelists.size()));
	}
	// Send information about other duelists to new client
	for(auto& kv : duelists)
	{
		if(kv.second == &client)
			continue; // Skip the new duelist itself
		client.Send(MakeHsPlayerEnter(*kv.second, t1max));
		uint8_t pos2 = MakeClientPos(kv.second->Position(), t1max);
		client.Send(MakeHsPlayerChangeReady(pos2, kv.second->Ready()));
	}
}

void Room::JoinToDuel(Client& client)
{
	// TODO
}

void Room::PostUnregisterFromOwner()
{
	asio::post(strand,
	[this, self = shared_from_this()]()
	{
		owner.Remove(options.id);
	});
}

void Room::SendToAll(const YGOPro::STOCMsg& msg)
{
	for(auto& c : clients)
		c->Send(msg);
}

} // namespace Multirole

} // namespace Ignis
