#include "../Context.hpp"

#include "../../YGOPro/Constants.hpp"

namespace Ignis::Multirole::Room
{

StateOpt Context::operator()(State::RockPaperScissor& /*unused*/) noexcept
{
	SendRPS();
	return std::nullopt;
}

StateOpt Context::operator()(State::RockPaperScissor& s, const Event::ChooseRPS& e) noexcept
{
	const auto& pos = e.client.Position();
	if(pos.second != 0U || e.value > 3U)
		return std::nullopt;
	s.choices[pos.first] = e.value;
	if((s.choices[0U] == 0U) || (s.choices[1U] == 0U))
		return std::nullopt;
	const auto rpsChoice1 = MakeRPSResult(s.choices[0U], s.choices[1U]);
	SendToTeam(0U, rpsChoice1);
	SendToTeam(1U, MakeRPSResult(s.choices[1U], s.choices[0U]));
	SendToSpectators(rpsChoice1);
	if(s.choices[0U] == s.choices[1U])
		return State::RockPaperScissor{};
	enum : uint8_t
	{
		ROCK    = 2U,
		PAPER   = 3U,
		SCISSOR = 1U,
	};
	return State::ChoosingTurn{
		duelists[{
		static_cast<uint8_t>(
			(s.choices[1U] == ROCK    && s.choices[0U] == SCISSOR) ||
			(s.choices[1U] == PAPER   && s.choices[0U] == ROCK)    ||
			(s.choices[1U] == SCISSOR && s.choices[0U] == PAPER)
		),0U}]};
}

StateOpt Context::operator()(State::RockPaperScissor& /*unused*/, const Event::ConnectionLost& e) noexcept
{
	const auto p = e.client.Position();
	if(p == Client::POSITION_SPECTATOR)
	{
		spectators.erase(&e.client);
		return std::nullopt;
	}
	uint8_t winner = 1U - GetSwappedTeam(p.first);
	SendToAll(MakeGameMsg({MSG_WIN, winner, WIN_REASON_CONNECTION_LOST}));
	SendToAll(MakeDuelEnd());
	return State::Closing{};
}

StateOpt Context::operator()(State::RockPaperScissor& /*unused*/, const Event::Join& e) noexcept
{
	SetupAsSpectator(e.client);
	e.client.Send(MakeDuelStart());
	return std::nullopt;
}

// Sends Rock, Paper, Scissor hand selection to the first player of each team
void Context::SendRPS() noexcept
{
	auto msg = MakeAskRPS();
	duelists[{0U, 0U}]->Send(msg);
	duelists[{1U, 0U}]->Send(msg);
}


} // namespace Ignis::Multirole::Room
