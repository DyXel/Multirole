#include "../Context.hpp"

namespace Ignis::Multirole::Room
{

StateOpt Context::operator()(State::Rematching& /*unused*/)
{
	SendToAll(MakeRematchWait());
	const auto msg = MakeAskIfRematch();
	SendToTeam(0, msg);
	SendToTeam(1, msg);
	return std::nullopt;
}

StateOpt Context::operator()(State::Rematching&, const Event::ConnectionLost& e)
{
	if(e.client.Position() == Client::POSITION_SPECTATOR)
		return std::nullopt;
	return State::Closing{};
}

StateOpt Context::operator()(State::Rematching& s, const Event::Rematch& e)
{
	if(e.client.Position() == Client::POSITION_SPECTATOR)
		return std::nullopt;
	if(!e.answer && s.answered.count(&e.client) == 0)
	{
		return State::Closing{};
	}
	else if(e.answer && s.answered.count(&e.client) == 0)
	{
		s.answered.insert(&e.client);
		if(++s.answerCount == duelists.size())
			return State::ChoosingTurn{s.turnChooser};
	}
	return std::nullopt;
}

} // namespace Ignis::Multirole::Room
