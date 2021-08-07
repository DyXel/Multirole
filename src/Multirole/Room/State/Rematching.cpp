#include "../Context.hpp"

namespace Ignis::Multirole::Room
{

StateOpt Context::operator()(State::Rematching& /*unused*/) noexcept
{
	SendToAll(MakeRematchWait());
	const auto msg = MakeAskIfRematch();
	SendToTeam(0U, msg);
	SendToTeam(1U, msg);
	return std::nullopt;
}

StateOpt Context::operator()(State::Rematching& /*unused*/, const Event::ConnectionLost& e) noexcept
{
	if(e.client.Position() == Client::POSITION_SPECTATOR)
	{
		spectators.erase(&e.client);
		return std::nullopt;
	}
	SendToAll(MakeDuelEnd());
	return State::Closing{};
}

StateOpt Context::operator()(State::Rematching& /*unused*/, const Event::Join& e) noexcept
{
	SetupAsSpectator(e.client);
	e.client.Send(MakeDuelStart());
	e.client.Send(MakeRematchWait());
	return std::nullopt;
}

StateOpt Context::operator()(State::Rematching& s, const Event::Rematch& e) noexcept
{
	if(e.client.Position() == Client::POSITION_SPECTATOR)
		return std::nullopt;
	if(!e.answer && s.answered.count(&e.client) == 0U)
	{
		SendToAll(MakeDuelEnd());
		return State::Closing{};
	}
	if(e.answer && s.answered.count(&e.client) == 0U)
	{
		s.answered.insert(&e.client);
		if(s.answered.size() == duelists.size())
		{
			SendToAll(MakeDuelStart());
			return State::ChoosingTurn{s.turnChooser};
		}
	}
	return std::nullopt;
}

} // namespace Ignis::Multirole::Room
