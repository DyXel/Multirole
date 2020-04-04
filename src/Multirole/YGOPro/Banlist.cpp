#include "Banlist.hpp"

namespace YGOPro
{

Banlist::Banlist(CodeSet&& whit, CodeSet&& semi, CodeSet&& limi, CodeSet&& forb) :
	whitelist(std::move(whit)),
	semilimited(std::move(semi)),
	limited(std::move(limi)),
	forbidden(std::move(forb))
{}

bool Banlist::IsWhitelist() const
{
	return !whitelist.empty();
}

const CodeSet& Banlist::Whitelist() const
{
	return whitelist;
}

const CodeSet& Banlist::Semilimited() const
{
	return semilimited;
}

const CodeSet& Banlist::Limited() const
{
	return limited;
}

const CodeSet& Banlist::Forbidden() const
{
	return forbidden;
}

} // namespace YGOPro
