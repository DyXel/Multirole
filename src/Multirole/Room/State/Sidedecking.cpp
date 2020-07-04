#include "../Context.hpp"

#include <algorithm>

#include "../../YGOPro/Constants.hpp"

namespace Ignis::Multirole::Room
{

StateOpt Context::operator()(State::Sidedecking& /*unused*/)
{
	const auto msg = MakeAskSidedeck();
	SendToTeam(0U, msg);
	SendToTeam(1U, msg);
	SendToSpectators(MakeSidedeckWait());
	return std::nullopt;
}

StateOpt Context::operator()(State::Sidedecking& /*unused*/, const Event::ConnectionLost& e)
{
	if(e.client.Position() == Client::POSITION_SPECTATOR)
		return std::nullopt;
	SendToAll(MakeDuelStart());
	uint8_t winner = 1U - GetSwappedTeam(e.client.Position().first);
	SendToAll(MakeGameMsg({MSG_WIN, winner, WIN_REASON_CONNECTION_LOST}));
	SendToAll(MakeDuelEnd());
	return State::Closing{};
}

StateOpt Context::operator()(State::Sidedecking& s, const Event::UpdateDeck& e)
{
	if(e.client.Position() == Client::POSITION_SPECTATOR)
		return std::nullopt;
	if(s.sidedecked.count(&e.client) == 0U)
	{
		// NOTE: assuming client original deck is always valid here
		const auto ogDeck = e.client.OriginalDeck();
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
			// TODO: Send error?
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
