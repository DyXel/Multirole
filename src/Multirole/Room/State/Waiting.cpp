#include "../Context.hpp"

namespace Ignis::Multirole::Room
{

StateOpt Context::operator()(State::Waiting& s, const Event::Join& e)
{
	if(s.host == nullptr)
		s.host = &e.client;
	e.client.Send(YGOPro::STOCMsg::JoinGame{hostInfo}); // TODO: create on ctor?
	std::lock_guard<std::mutex> lock(mDuelists);
	if(TryEmplaceDuelist(e.client))
	{
		SendToAll(MakePlayerEnter(e.client));
		SendToAll(MakePlayerChange(e.client));
		e.client.Send(MakeTypeChange(e.client, s.host == &e.client));
		e.client.Send(MakeWatchChange(spectators.size()));
	}
	else
	{
		spectators.insert(&e.client);
		SendToAll(MakeWatchChange(spectators.size()));
	}
	for(const auto& kv : duelists)
	{
		if(kv.second == &e.client)
			continue; // Skip the new duelist itself
		e.client.Send(MakePlayerEnter(*kv.second));
		e.client.Send(MakePlayerChange(*kv.second));
	}
	return std::nullopt;
}

StateOpt Context::operator()(State::Waiting& s, const Event::ConnectionLost& e)
{
	if(s.host == &e.client)
		return State::Closing{};
	e.client.Disconnect();
	if(const auto p = e.client.Position(); p != Client::POSITION_SPECTATOR)
	{
		{
			std::lock_guard<std::mutex> lock(mDuelists);
			duelists.erase(p);
		}
		SendToAll(MakePlayerChange(e.client, PCHANGE_TYPE_LEAVE));
	}
	else
	{
		spectators.erase(&e.client);
		SendToAll(MakeWatchChange(spectators.size()));
	}
	return std::nullopt;
}

StateOpt Context::operator()(State::Waiting& s, const Event::ToObserver& e)
{
	const auto p = e.client.Position();
	if(p == Client::POSITION_SPECTATOR)
		return std::nullopt;
	{
		std::lock_guard<std::mutex> lock(mDuelists);
		duelists.erase(p);
	}
	spectators.insert(&e.client);
	SendToAll(MakePlayerChange(e.client, PCHANGE_TYPE_SPECTATE));
	e.client.SetPosition(Client::POSITION_SPECTATOR);
	e.client.Send(MakeTypeChange(e.client, s.host == &e.client));
	return std::nullopt;
}

StateOpt Context::operator()(State::Waiting& s, const Event::ToDuelist& e)
{
	const auto p = e.client.Position();
	std::lock_guard<std::mutex> lock(mDuelists);
	if(p == Client::POSITION_SPECTATOR)
	{
		// NOTE: ifs intentionally not short-circuited
		if(TryEmplaceDuelist(e.client))
		{
			spectators.erase(&e.client);
			SendToAll(MakePlayerEnter(e.client));
			SendToAll(MakePlayerChange(e.client));
			SendToAll(MakeWatchChange(spectators.size()));
			e.client.Send(MakeTypeChange(e.client, s.host == &e.client));
		}
	}
	else
	{
		duelists.erase(p);
		auto nextPos = p;
		nextPos.second++;
		if(TryEmplaceDuelist(e.client, nextPos) && e.client.Position() != p)
		{
			e.client.SetReady(false);
			SendToAll(MakePlayerChange(p, e.client.Position()));
			SendToAll(MakePlayerChange(e.client));
			e.client.Send(MakeTypeChange(e.client, s.host == &e.client));
		}
	}
	return std::nullopt;
}

StateOpt Context::operator()(State::Waiting& /*unused*/, const Event::Ready& e)
{
	if(e.client.Position() == Client::POSITION_SPECTATOR ||
	   e.client.Ready() == e.value)
		return std::nullopt;
	bool value = e.value;
	if(e.client.OriginalDeck() == nullptr)
		value = false;
	if(value && hostInfo.dontCheckDeck == 0)
	{
		if(auto error = CheckDeck(*e.client.OriginalDeck()); error)
		{
			e.client.Send(*error);
			value = false;
		}
	}
	e.client.SetReady(value);
	SendToAll(MakePlayerChange(e.client));
	return std::nullopt;
}

StateOpt Context::operator()(State::Waiting& /*unused*/, const Event::UpdateDeck& e)
{
	if(e.client.Position() == Client::POSITION_SPECTATOR)
		return std::nullopt;
	e.client.SetOriginalDeck(LoadDeck(e.main, e.side));
	return std::nullopt;
}

StateOpt Context::operator()(State::Waiting& s, const Event::TryKick& e)
{
	if(s.host != &e.client)
		return std::nullopt;
	Client::PosType p;
	p.first = static_cast<unsigned char>(e.pos >= hostInfo.t1Count);
	p.second = p.first != 0U ? e.pos - hostInfo.t1Count : e.pos;
	if(duelists.count(p) == 0 || duelists[p] == s.host)
		return std::nullopt;
	Client* kicked = duelists[p];
	kicked->Disconnect();
	{
		std::lock_guard<std::mutex> lock(mDuelists);
		duelists.erase(p);
	}
	SendToAll(MakePlayerChange(*kicked, PCHANGE_TYPE_LEAVE));
	return std::nullopt;
}

StateOpt Context::operator()(State::Waiting& s, const Event::TryStart& e)
{
	auto ValidateDuelistsSetup = [&]() -> bool
	{
		if((hostInfo.duelFlags & 0x80) == 0) // NOLINT: DUEL_RELAY
		{
			if(int32_t(duelists.size()) == hostInfo.t1Count + hostInfo.t2Count)
				return true;
			return false;
		}
		auto HasAtLeastOneDuelistOnEachTeam = [&]() -> bool
		{
			std::array<uint8_t, 2> counts{};
			for(const auto& kv : duelists)
				counts[kv.first.first]++;
			return counts[0] > 0 && counts[1] > 0;
		};
		if(!HasAtLeastOneDuelistOnEachTeam())
			return false;
		// At this point it has been decided that this relay setup is
		// valid, however we need to move the duelists to their first
		// positions that are available rather than having them scrambled.
		// TODO
		return true;
	};
	if(s.host != &e.client)
		return std::nullopt;
	for(const auto& kv : duelists)
		if(!kv.second->Ready())
			return std::nullopt;
	if(!ValidateDuelistsSetup())
		return std::nullopt;
	SendToAll(MakeStartDuel());
	return State::RockPaperScissor{};
}

// private

bool Context::TryEmplaceDuelist(Client& client, Client::PosType hint)
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
	if(hint.first == 0U)
		if(EmplaceLoop(hint, hostInfo.t1Count))
			return true;
	auto p = hint;
	if(hint.first != 1U)
		p.second = 0U;
	p.first = 1U;
	if(EmplaceLoop(p, hostInfo.t2Count))
		return true;
	if(hint != Client::PosType{})
		return TryEmplaceDuelist(client);
	return false;
}

} // namespace Ignis::Multirole::Room
