#include "../Context.hpp"

namespace Ignis::Multirole::Room
{

void Context::operator()(State::RockPaperScissor&)
{
	SendRPS();
}

StateOpt Context::operator()(State::RockPaperScissor& s, Event::ChooseRPS& e)
{
	const auto& pos = e.client.Position();
	if(pos.second != 0u || e.value > 3)
		return std::nullopt;
	s.choices[pos.first] = e.value;
	if(!s.choices[0] || !s.choices[1])
		return std::nullopt;
	SendToTeam(0u, MakeRPSResult(s.choices[0], s.choices[1]));
	SendToTeam(1u, MakeRPSResult(s.choices[1], s.choices[0]));
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
		),0u}]};
}

// Sends Rock, Paper, Scissor hand selection to the first player of each team
void Context::SendRPS()
{
	auto msg = MakeAskRPS();
	duelists[{0u, 0u}]->Send(msg);
	duelists[{1u, 0u}]->Send(msg);
}


} // namespace Ignis::Multirole::Room
