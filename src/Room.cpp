#include "Room.hpp"

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

// Shorthand to create a HsWatchChange message with number of spectators
YGOPro::STOCMsg MakeHsWatchChange(std::size_t count)
{
	return {YGOPro::STOCMsg::HsWatchChange{static_cast<uint16_t>(count)}};
}

// NOTE: exists on C++20 standard
// template<class Key, class T, class Compare, class Alloc, class Pred>
// void EraseIf(std::map<Key, T, Compare, Alloc>& c, Pred pred)
// {
// 	for(auto it = c.begin(), last = c.end(); it != last;)
// 	{
// 		if(pred(*it))
// 			it = c.erase(it);
// 		else
// 			++it;
// 	}
// }

Room::Room(IRoomManager& owner, asio::io_context& ioCtx, Options options) :
	owner(owner),
	strand(ioCtx),
	removingSelf(false),
	options(std::move(options)),
	state(WAITING)
{}

Room::StateEnum Room::State() const
{
	return state;
}

bool Room::CheckPassword(std::string_view str) const
{
	return options.pass == "" || options.pass == str;
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
		std::lock_guard<std::mutex> lock(mClients);
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

void Room::Start()
{
	options.id = owner.Add(shared_from_this());
}

void Room::Stop()
{
	state = STOPPING;
	// TODO: stop timers too
	std::lock_guard<std::mutex> lock(mClients);
	host.reset();
	for(auto& kv : duelists)
		kv.second->Stop();
	for(auto& c : spectators)
		c->Stop();
}

// private

void Room::Add(std::shared_ptr<Client> client)
{
	using YGOPro::STOCMsg;
	std::lock_guard<std::mutex> lock(mClients);
	if(!host)
		host = client;
	client->Send(STOCMsg(STOCMsg::CreateGame{options.id}));
	if(state != WAITING)
	{
		spectators.insert(client);
		// TODO: CatchUp
		return;
	}
	client->Send(STOCMsg::JoinGame{options.info}); // TODO: create on ctor?
	auto TryEmplaceDuelist = [&](auto c, uint8_t m1, uint8_t m2)
	{
		Client::PositionType p{0u, 0u};
		for(; p.second < m1; p.second++)
		{
			if(duelists.count(p) == 0 && duelists.emplace(p, c).second)
			{
				c->SetPosition(p);
				return true;
			}
		}
		p.first = 1u;
		for(p.second = 0u; p.second < m2; p.second++)
		{
			if(duelists.count(p) == 0 && duelists.emplace(p, c).second)
			{
				c->SetPosition(p);
				return true;
			}
		}
		return false;
	};
	if(TryEmplaceDuelist(client, options.info.t1Count, options.info.t2Count))
	{
		auto MakeHsPlayerEnter = [](auto c, uint8_t m1)
		{
			STOCMsg::HsPlayerEnter proto{};
			using namespace StringUtils;
			UTF16ToBuffer(proto.name, UTF8ToUTF16(c->Name()));
			proto.pos = MakeClientPos(c->Position(), m1);
			return STOCMsg(proto);
		};
		auto MakeHsPlayerChange = [](auto c, uint8_t m1)
		{
			STOCMsg::HsPlayerChange proto{};
			const PlayerChange pc = c->Ready() ? READY : NOT_READY;
			const auto pos = MakeClientPos(c->Position(), m1);
			proto.status = MakeClientStatus(pos, pc);
			return STOCMsg(proto);
		};
		// Inform new duelist if they are the host or not
		uint8_t pos = MakeClientPos(client->Position(), options.info.t1Count);
		client->Send(STOCMsg::TypeChange{MakeClientHost(pos, host == client)});
		// Send information about new duelist to all clients
		SendToAll(MakeHsPlayerEnter(client, options.info.t1Count));
		SendToAll(MakeHsPlayerChange(client, options.info.t1Count));
		// Send information about other duelists to new duelist
		for(auto& kv : duelists)
		{
			if(kv.second == client)
				continue; // Skip the new duelist itself
			client->Send(MakeHsPlayerEnter(kv.second, options.info.t1Count));
			client->Send(MakeHsPlayerChange(kv.second, options.info.t1Count));
		}
		// Send spectator count to new duelist
		client->Send(MakeHsWatchChange(spectators.size()));
	}
	else
	{
		spectators.insert(client);
		SendToAll(MakeHsWatchChange(spectators.size()));
	}
}

void Room::Remove(std::shared_ptr<Client> client)
{
	std::lock_guard<std::mutex> lock(mClients);
	if(host == client)
		host.reset();

	if(spectators.erase(client) == 0u)
	{
		duelists.erase(client->Position());
		// TODO: Send update to the rest of the clients
	}
	else if(state == WAITING)
	{
		SendToAll(MakeHsWatchChange(spectators.size()));
	}

	if(duelists.empty() && spectators.empty() && !host)
		PostRemoveSelf();
}

void Room::PostRemoveSelf()
{
	if(removingSelf)
		return;
	removingSelf = true;
	asio::post(strand,
	[this, self = shared_from_this()]()
	{
		owner.Remove(options.id);
	});
}

void Room::SendToAll(const YGOPro::STOCMsg& msg)
{
	for(auto& kv : duelists)
		kv.second->Send(msg);
	for(auto& c : spectators)
		c->Send(msg);
}

// void Room::SendToAllExcept(std::shared_ptr<Client> client, const YGOPro::STOCMsg& msg)
// {
// 	for(auto& kv : duelists)
// 		if(kv.second != client)
// 			kv.second->Send(msg);
// 	for(auto& c : spectators)
// 		if(c != client)
// 			c->Send(msg);
// }

} // namespace Multirole

} // namespace Ignis
