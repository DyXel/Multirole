#include "../Context.hpp"

namespace Ignis::Multirole::Room
{

void Context::operator()(State::Closing&)
{
	for(const auto& kv : duelists)
		kv.second->Disconnect();
	for(const auto& c : spectators)
		c->Disconnect();
	spectators.clear();
	if(std::lock_guard<std::mutex> lock(mDuelists); true)
		duelists.clear();
}

} // namespace Ignis::Multirole::Room
