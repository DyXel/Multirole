#include "Room.hpp"

#include <fmt/format.h>

#include "Client.hpp"
#include "IRoomManager.hpp"

namespace Ignis
{

namespace Multirole
{

// public

Room::Room(IRoomManager& owner, asio::io_context& ioCtx, Options options) :
	STOCMsgFactory(options.info.t1Count),
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
			auto pos = EncodePosition(kv.first);
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
			{
				std::lock_guard<std::mutex> lock(mDuelists);
				duelists.erase(posKey);
			}
			SendToAll(MakePlayerChange(client, PCHANGE_TYPE_LEAVE));
		}
		else
		{
			SendToAll(MakeWatchChange(clients.size() - duelists.size() - 1u));
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
		SendToAll(MakeChat(client, str));
	}
	else
	{
		const auto fmtStr = fmt::format(FMT_STRING("{:s}: {:s}"), client.Name(), str);
		SendToAll(MakeChat(CHAT_MSG_TYPE_SPECTATOR, fmtStr));
	}
}

void Room::OnToDuelist(Client& client)
{
	if(state != WAITING)
		return;
	const auto posKey = client.Position();
	std::lock_guard<std::mutex> lock(mDuelists);
	if(posKey == Client::POSITION_SPECTATOR)
	{
		// NOTE: ifs intentionally not short-circuited
		if(TryEmplaceDuelist(client))
		{
			SendToAll(MakePlayerEnter(client));
			SendToAll(MakePlayerChange(client));
			SendToAll(MakeWatchChange(clients.size() - duelists.size()));
			client.Send(MakeTypeChange(client, host == &client));
		}
	}
	else
	{
		duelists.erase(posKey);
		auto nextPos = posKey;
		nextPos.second++;
		if(TryEmplaceDuelist(client, nextPos) && client.Position() != posKey)
		{
			client.SetReady(false);
			SendToAll(MakePlayerChange(posKey, client.Position()));
			SendToAll(MakePlayerChange(client));
			client.Send(MakeTypeChange(client, host == &client));
		}
	}
}

void Room::OnToObserver(Client& client)
{
	if(state != WAITING)
		return;
	const auto posKey = client.Position();
	if(posKey == Client::POSITION_SPECTATOR)
		return;
	{
		std::lock_guard<std::mutex> lock(mDuelists);
		duelists.erase(posKey);
	}
	SendToAll(MakePlayerChange(client, PCHANGE_TYPE_SPECTATE));
	client.SetPosition(Client::POSITION_SPECTATOR);
	client.Send(MakeTypeChange(client, host == &client));
}

void Room::OnReady(Client& client, bool value)
{
	if(state != WAITING)
		return;
	if(client.Position() == Client::POSITION_SPECTATOR)
		return;
	client.SetReady(value);
	SendToAll(MakePlayerChange(client));
}

void Room::OnTryKick(Client& client, uint8_t pos)
{
	if(state != WAITING || &client != host)
		return;
	Client::PosType p;
	p.first = pos >= options.info.t1Count;
	p.second = p.first ? pos - options.info.t1Count : pos;
	if(duelists.count(p) == 0 || duelists[p] == host)
		return;
	Client* kicked = duelists[p];
	kicked->Disconnect();
	{
		std::lock_guard<std::mutex> lock(mDuelists);
		duelists.erase(p);
	}
	SendToAll(MakePlayerChange(*kicked, PCHANGE_TYPE_LEAVE));
}

void Room::OnTryStart(Client& client)
{
	if(state != WAITING || &client != host)
		return;
	for(const auto& kv : duelists)
		if(!kv.second->Ready())
			return;
	// TODO
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

bool Room::TryEmplaceDuelist(Client& client, Client::PosType hint)
{
	auto EmplaceLoop = [&](Client::PosType p, uint8_t max) -> bool
	{
		for(; p.second < max; p.second++)
		{
			if(duelists.count(p) == 0 && duelists.emplace(p, &client).second)
			{
				client.SetPosition(p);
				return true;
			}
		}
		return false;
	};
	if(hint.first == 0u)
		if(EmplaceLoop(hint, options.info.t1Count))
			return true;
	auto p = hint;
	if(hint.first != 1u)
		p.second = 0u;
	p.first = 1u;
	if(EmplaceLoop(p, options.info.t2Count))
		return true;
	if(hint != Client::PosType{})
		return TryEmplaceDuelist(client);
	return false;
}

void Room::JoinToWaiting(Client& client)
{
	if(host == nullptr)
		host = &client;
	using YGOPro::STOCMsg;
	client.Send(STOCMsg::JoinGame{options.info}); // TODO: create on ctor?
	std::lock_guard<std::mutex> lock(mDuelists);
	if(TryEmplaceDuelist(client))
	{
		SendToAll(MakePlayerEnter(client));
		SendToAll(MakePlayerChange(client));
		client.Send(MakeTypeChange(client, host == &client));
		client.Send(MakeWatchChange(clients.size() - duelists.size()));
	}
	else
	{
		SendToAll(MakeWatchChange(clients.size() - duelists.size()));
	}
	for(auto& kv : duelists)
	{
		if(kv.second == &client)
			continue; // Skip the new duelist itself
		client.Send(MakePlayerEnter(*kv.second));
		client.Send(MakePlayerChange(*kv.second));
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
