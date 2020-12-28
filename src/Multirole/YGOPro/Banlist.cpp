#include "Banlist.hpp"

namespace YGOPro
{

Banlist::Banlist(bool whitelist, DictType dict) :
	whitelist(whitelist),
	dict(std::move(dict))
{}

bool Banlist::IsWhitelist() const
{
	return whitelist;
}

const Banlist::DictType& Banlist::Dict() const
{
	return dict;
}

} // namespace YGOPro
