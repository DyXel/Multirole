#include "../Context.hpp"

namespace Ignis::Multirole::Room
{

void Context::operator()(State::RockPaperScissor& /*unused*/)
{
	SendRPS();
}

StateOpt Context::operator()(State::RockPaperScissor& s, Event::ChooseRPS& e)
{
	const auto& pos = e.client.Position();
	if(pos.second != 0U || e.value > 3)
		return std::nullopt;
	s.choices[pos.first] = e.value;
	if((s.choices[0] == 0u) || (s.choices[1] == 0u))
		return std::nullopt;
	SendToTeam(0U, MakeRPSResult(s.choices[0], s.choices[1]));
	SendToTeam(1U, MakeRPSResult(s.choices[1], s.choices[0]));
	if(s.choices[0] == s.choices[1])
		return State::RockPaperScissor{};
	enum : uint8_t
	{
		ROCK    = 2,
		PAPER   = 3,
		SCISSOR = 1,
	};
	return State::ChoosingTurn{
		duelists[{
		static_cast<uint8_t>(
			(s.choices[1] == ROCK    && s.choices[0] == SCISSOR) ||
			(s.choices[1] == PAPER   && s.choices[0] == ROCK)    ||
			(s.choices[1] == SCISSOR && s.choices[0] == PAPER)
		),0U}]};
}

// Sends Rock, Paper, Scissor hand selection to the first player of each team
void Context::SendRPS()
{
	auto msg = MakeAskRPS();
	duelists[{0U, 0U}]->Send(msg);
	duelists[{1U, 0U}]->Send(msg);
}


} // namespace Ignis::Multirole::Room
