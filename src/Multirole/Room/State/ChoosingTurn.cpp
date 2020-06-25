#include "../Context.hpp"

namespace Ignis::Multirole::Room
{

void Context::operator()(State::ChoosingTurn& s)
{
	s.turnChooser->Send(MakeAskIfGoingFirst());
}

StateOpt Context::operator()(State::ChoosingTurn& s, const Event::ChooseTurn& e)
{
	if(s.turnChooser != &e.client)
		return std::nullopt;
	return State::Dueling{
		static_cast<uint8_t>(
			(e.client.Position().first == 0U && !e.goingFirst) ||
			(e.client.Position().first == 1U && e.goingFirst)
		),
		nullptr,
		{0u, 0u},
		nullptr,
		std::nullopt};
}

} // namespace Ignis::Multirole::Room
