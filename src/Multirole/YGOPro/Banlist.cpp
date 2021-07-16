#include "Banlist.hpp"

namespace YGOPro
{

Banlist::Banlist(bool whitelist, DictType dict) noexcept :
	whitelist(whitelist),
	dict(std::move(dict))
{}

bool Banlist::IsWhitelist() const noexcept
{
	return whitelist;
}

const Banlist::DictType& Banlist::Dict() const noexcept
{
	return dict;
}

} // namespace YGOPro
