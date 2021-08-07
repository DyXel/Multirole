#include "../Context.hpp"

#include <algorithm> // std::equal

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
		// NOTE: assuming client original deck is always valid here
		const auto* ogDeck = e.client.OriginalDeck();
		auto sideDeck = LoadDeck(e.main, e.side);
		const auto ogMap = ogDeck->GetCodeMap();
		const auto sideMap = sideDeck->GetCodeMap();
		// A full side deck is considered valid in respect to the
		// original full deck if:
		//	the number of cards in each deck is equal to the original's
		//	the sum of card codes is equal to the original's
		if(ogDeck->Main().size() == sideDeck->Main().size() &&
		   ogDeck->Extra().size() == sideDeck->Extra().size() &&
		   ogDeck->Side().size() == sideDeck->Side().size() &&
		   std::equal(ogMap.begin(), ogMap.end(), sideMap.begin()))
		{
			e.client.SetCurrentDeck(std::move(sideDeck));
			s.sidedecked.insert(&e.client);
			e.client.Send(MakeDuelStart());
		}
		else
		{
			e.client.Send(MakeSideError());
		}
		if(s.sidedecked.size() == duelists.size())
		{
			SendToSpectators(MakeDuelStart());
			return State::ChoosingTurn{s.turnChooser};
		}
	}
	return std::nullopt;
}

} // namespace Ignis::Multirole::Room
