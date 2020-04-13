#include "../Context.hpp"

namespace Ignis::Multirole::Room
{

void Context::operator()(State::Closing& /*unused*/)
{
	for(const auto& kv : duelists)
		kv.second->Disconnect();
	for(const auto& c : spectators)
		c->Disconnect();
	spectators.clear();
	duelists.clear();
}

} // namespace Ignis::Multirole::Room
