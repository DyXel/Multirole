#include "../Context.hpp"

namespace Ignis::Multirole::Room
{

StateOpt Context::operator()(State::Closing& /*unused*/)
{
	for(const auto& kv : duelists)
		kv.second->Disconnect();
	for(const auto& c : spectators)
		c->Disconnect();
	spectators.clear();
	duelists.clear();
	return std::nullopt;
}

StateOpt Context::operator()(State::Closing& /*unused*/, const Event::Join& e)
{
	e.client.Disconnect();
	return std::nullopt;
}

StateOpt Context::operator()(State::Closing& /*unused*/, const Event::ConnectionLost& e)
{
	e.client.Disconnect();
	return std::nullopt;
}

} // namespace Ignis::Multirole::Room
