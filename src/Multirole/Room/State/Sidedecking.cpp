#include "../Context.hpp"

#include <algorithm> // std::equal

#include "../../YGOPro/CardDatabase.hpp"
#include "../../YGOPro/Constants.hpp"

namespace Ignis::Multirole::Room
{

StateOpt Context::operator()(State::Sidedecking& /*unused*/) noexcept
{
	const auto msg = MakeAskSidedeck();
	SendToTeam(0U, msg);
	SendToTeam(1U, msg);
	SendToSpectators(MakeSidedeckWait());
	return std::nullopt;
}

StateOpt Context::operator()(State::Sidedecking& /*unused*/, const Event::ConnectionLost& e) noexcept
{
	const auto p = e.client.Position();
	if(p == Client::POSITION_SPECTATOR)
	{
		spectators.erase(&e.client);
		return std::nullopt;
	}
	SendToAll(MakeDuelStart());
	uint8_t winner = 1U - GetSwappedTeam(p.first);
	SendToAll(MakeGameMsg({MSG_WIN, winner, WIN_REASON_CONNECTION_LOST}));
	SendToAll(MakeDuelEnd());
	return State::Closing{};
}

StateOpt Context::operator()(State::Sidedecking& /*unused*/, const Event::Join& e) noexcept
{
	SetupAsSpectator(e.client);
	e.client.Send(MakeDuelStart());
	e.client.Send(MakeSidedeckWait());
	return std::nullopt;
}

StateOpt Context::operator()(State::Sidedecking& s, const Event::UpdateDeck& e) noexcept
{
	if(e.client.Position() == Client::POSITION_SPECTATOR)
		return std::nullopt;
	if(s.sidedecked.count(&e.client) == 0U)
	{
		auto CountSkills = [this](const auto& map)
		{
			std::size_t count = 0;
			for(const auto code : map)
				count += (cdb->DataFromCode(code).type & TYPE_SKILL) != 0;
			return count;
		};
		auto CountLegends = [this](const auto& deck)
		{
			std::size_t count = 0;
			for(const auto code : deck->Main())
				count += (cdb->ExtraFromCode(code).scope & SCOPE_LEGEND) != 0;
			for(const auto code : deck->Extra())
				count += (cdb->ExtraFromCode(code).scope & SCOPE_LEGEND) != 0;
			return count;
		};
		auto SidedeckingError = [&]()
		{
			e.client.Send(MakeSideError());
			return std::nullopt;
		};
		// NOTE: Assuming client original deck is always valid here.
		const auto* ogDeck = e.client.OriginalDeck();
		auto sideDeck = LoadDeck(e.main, e.side);
		const auto oldLegends = CountLegends(ogDeck);
		const auto newLegends = CountLegends(sideDeck);
		// Ideally the check should be only newSkills/Legends > 1, but a player
		// might host with "don't check deck" and have more than 1 skill/LEGEND.
		// This check ensures that the sided deck will always be valid in such
		// case and prevent softlocking.
		if(newLegends > std::max<std::size_t>(oldLegends, 1U))
			return SidedeckingError();
		const auto oldSkills = CountSkills(ogDeck->Main());
		const auto newSkills = CountSkills(sideDeck->Main());
		if(newSkills > std::max<std::size_t>(oldSkills, 1U))
			return SidedeckingError();
		// A full side deck is considered valid in respect to the
		// original full deck if:
		//	the number of cards in each deck is equal to the original's
		//	the sum of card codes is equal to the original's
		if((ogDeck->Main().size() - oldSkills) != (sideDeck->Main().size() - newSkills))
			return SidedeckingError();
		if(ogDeck->Extra().size() != sideDeck->Extra().size())
			return SidedeckingError();
		const auto ogMap = ogDeck->GetCodeMap();
		const auto sideMap = sideDeck->GetCodeMap();
		if(!std::equal(ogMap.begin(), ogMap.end(), sideMap.begin()))
			return SidedeckingError();
		e.client.SetCurrentDeck(std::move(sideDeck));
		s.sidedecked.insert(&e.client);
		e.client.Send(MakeDuelStart());
		if(s.sidedecked.size() == duelists.size())
		{
			SendToSpectators(MakeDuelStart());
			return State::ChoosingTurn{s.turnChooser};
		}
	}
	return std::nullopt;
}

} // namespace Ignis::Multirole::Room
