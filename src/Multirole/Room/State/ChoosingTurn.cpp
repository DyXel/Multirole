#include "../Context.hpp"

#include "../../YGOPro/Constants.hpp"

namespace Ignis::Multirole::Room
{

StateOpt Context::operator()(State::ChoosingTurn& s)
{
	s.turnChooser->Send(MakeAskIfGoingFirst());
	return std::nullopt;
}

StateOpt Context::operator()(State::ChoosingTurn& s, const Event::ChooseTurn& e)
{
	if(s.turnChooser != &e.client)
		return std::nullopt;
	isTeam1GoingFirst = static_cast<uint8_t>(
			(e.client.Position().first == 0U && !e.goingFirst) ||
			(e.client.Position().first == 1U && e.goingFirst));
	return State::Dueling{nullptr, {0U, 0U}, nullptr, std::nullopt};
}

StateOpt Context::operator()(State::ChoosingTurn& /*unused*/, const Event::ConnectionLost& e)
{
	if(e.client.Position() == Client::POSITION_SPECTATOR)
		return std::nullopt;
	uint8_t winner = 1U - GetSwappedTeam(e.client.Position().first);
	SendToAll(MakeGameMsg({MSG_WIN, winner, WIN_REASON_CONNECTION_LOST}));
	SendToAll(MakeDuelEnd());
	return State::Closing{};
}

} // namespace Ignis::Multirole::Room
