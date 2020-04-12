#include "Context.hpp"

#include <fmt/format.h>

#include "../CardDatabase.hpp"
#include "../STOCMsgFactory.hpp"
#include "../YGOPro/Banlist.hpp"
#include "../YGOPro/Deck.hpp"
#include "../YGOPro/Scope.hpp"
#include "../YGOPro/Type.hpp"

namespace Ignis::Multirole::Room
{

Context::Context(
	YGOPro::HostInfo&& hostInfo,
	YGOPro::DeckLimits&& limits,
	CoreProvider::CorePkg&& cpkg,
	const YGOPro::Banlist* banlist)
	:
	STOCMsgFactory(hostInfo.t1Count),
	hostInfo(std::move(hostInfo)),
	limits(std::move(limits)),
	cpkg(std::move(cpkg)),
	banlist(banlist)
{}

const YGOPro::HostInfo& Context::HostInfo() const
{
	return hostInfo;
}

std::map<uint8_t, std::string> Context::GetDuelistsNames()
{
	std::map<uint8_t, std::string> ret;
	{
		std::lock_guard<std::mutex> lock(mDuelists);
		for(auto& kv : duelists)
			ret.emplace(EncodePosition(kv.first), kv.second->Name());
	}
	return ret;
}

void Context::SetId(uint32_t newId)
{
	id = newId;
}

// private

void Context::SendToTeam(uint8_t team, const YGOPro::STOCMsg& msg)
{
	assert(team < 2);
	for(const auto& kv : duelists)
	{
		if(kv.first.first != team)
			continue;
		kv.second->Send(msg);
	}
}

void Context::SendToSpectators(const YGOPro::STOCMsg& msg)
{
	for(const auto& c : spectators)
		c->Send(msg);
}

void Context::SendToAll(const YGOPro::STOCMsg& msg)
{
	for(const auto& kv : duelists)
		kv.second->Send(msg);
	SendToSpectators(msg);
}

void Context::MakeAndSendChat(Client& client, std::string_view msg)
{
	if(client.Position() != Client::POSITION_SPECTATOR)
	{
		SendToAll(MakeChat(client, msg));
	}
	else
	{
		const auto formatted =
			fmt::format(FMT_STRING("{:s}: {:s}"), client.Name(), msg);
		SendToAll(MakeChat(CHAT_MSG_TYPE_SPECTATOR, formatted));
	}
}

std::unique_ptr<YGOPro::Deck> Context::LoadDeck(
	const std::vector<uint32_t>& main,
	const std::vector<uint32_t>& side) const
{
	auto IsExtraDeckCardType = [](uint32_t type) constexpr -> bool
	{
		if(type & (TYPE_FUSION | TYPE_SYNCHRO | TYPE_XYZ))
			return true;
		// NOTE: Link Spells exist.
		if((type & TYPE_LINK) && (type & TYPE_MONSTER))
			return true;
		return false;
	};
	auto& db = *cpkg.db;
	YGOPro::CodeVector m, e, s;
	uint32_t err = 0;
	for(const auto code : main)
	{
		auto& data = db.DataFromCode(code);
		if(data.code == 0)
		{
			err = code;
			continue;
		}
		if(data.type & TYPE_TOKEN)
			continue;
		if(IsExtraDeckCardType(data.type))
			e.push_back(code);
		else
			m.push_back(code);
	}
	for(const auto code : side)
	{
		auto& data = db.DataFromCode(code);
		if(data.code == 0)
		{
			err = code;
			continue;
		}
		if(data.type & TYPE_TOKEN)
			continue;
		s.push_back(code);
	}
	return std::make_unique<YGOPro::Deck>(
		std::move(m),
		std::move(e),
		std::move(s),
		err);
}

std::unique_ptr<YGOPro::STOCMsg> Context::CheckDeck(const YGOPro::Deck& deck) const
{
	using namespace Error;
	using namespace YGOPro;
	// Handy shortcut.
	auto MakeErrorPtr = [](DeckOrCard type, uint32_t value)
	{
		return std::make_unique<STOCMsg>(MakeError(type, value));
	};
	// Check if the deck had any error while loading.
	if(deck.Error())
		return MakeErrorPtr(CARD_UNKNOWN, deck.Error());
	// Amalgamate all card codes into a single map for easier iteration.
	std::map<uint32_t, std::size_t> all;
	auto AddToMap = [&all](const CodeVector& from)
	{
		for(const auto& code : from)
			all[code]++;
	};
	AddToMap(deck.Main());
	AddToMap(deck.Extra());
	AddToMap(deck.Side());
	// Merge aliased cards to their original code and delete them
	auto& db = *cpkg.db;
	for(auto it = all.begin(), last = all.end(); it != last;)
	{
		if(uint32_t alias = db.DataFromCode(it->first).alias; alias != 0)
		{
			all[alias] = all[alias] + it->second;
			it = all.erase(it);
		}
		else
		{
			++it;
		}
	}
	// Check if the deck obeys the limits.
	auto OutOfBound = [](const auto& lim, const CodeVector& vector) -> auto
	{
		std::pair<std::size_t, bool> p{vector.size(), false};
		return (p.second = p.first < lim.min || p.first > lim.max), p;
	};
	if(const auto p = OutOfBound(limits.main, deck.Main()); p.second)
		return MakeErrorPtr(DECK_BAD_MAIN_COUNT, p.first);
	if(const auto p = OutOfBound(limits.extra, deck.Extra()); p.second)
		return MakeErrorPtr(DECK_BAD_EXTRA_COUNT, p.first);
	if(const auto p = OutOfBound(limits.side, deck.Side()); p.second)
		return MakeErrorPtr(DECK_BAD_SIDE_COUNT, p.first);
	// Custom predicates...
	//	true if card scope is unnofficial using currently allowed mode.
	auto CheckUnofficial = [](uint32_t scope, uint8_t allowed) constexpr -> bool
	{
		switch(allowed)
		{
		case ALLOWED_CARDS_OCG_ONLY:
		case ALLOWED_CARDS_TCG_ONLY:
		case ALLOWED_CARDS_OCG_TCG:
			return scope > SCOPE_OCG_TCG;
		}
		return false;
	};
	//	true if card scope is prerelease and they aren't allowed
	auto CheckPrelease = [](uint32_t scope, uint8_t allowed) constexpr -> bool
	{
		return allowed == ALLOWED_CARDS_WITH_PRERELEASE &&
		       !(scope & SCOPE_OFFICIAL);
	};
	//	true if only ocg are allowed and scope is not ocg (its tcg).
	auto CheckOCG = [](uint32_t scope, uint8_t allowed) constexpr -> bool
	{
		return allowed == ALLOWED_CARDS_OCG_ONLY && !(scope & SCOPE_OCG);
	};
	//	true if only tcg are allowed and scope is not tcg (its ocg).
	auto CheckTCG = [](uint32_t scope, uint8_t allowed) constexpr -> bool
	{
		return allowed == ALLOWED_CARDS_TCG_ONLY && !(scope & SCOPE_TCG);
	};
	//	true if card code exists on the banlist and exceeds the listed amount.
	auto CheckBanlist = [](const auto& kv, const Banlist& bl) -> bool
	{
		if(bl.IsWhitelist() && bl.Whitelist().count(kv.first) == 0)
			return true;
		if(bl.Forbidden().count(kv.first) != 0U)
			return true;
		if(kv.second > 1 && (bl.Limited().count(kv.first) != 0U))
			return true;
		if(kv.second > 2 && (bl.Semilimited().count(kv.first) != 0U))
			return true;
		return false;
	};
	for(const auto& kv : all)
	{
		if(kv.second > 3)
			return MakeErrorPtr(CARD_MORE_THAN_3, kv.first);
		if(db.DataFromCode(kv.first).type & hostInfo.forb)
			return MakeErrorPtr(CARD_FORBIDDEN_TYPE, kv.first);
		const auto& ced = db.ExtraFromCode(kv.first);
		if(CheckUnofficial(ced.scope, hostInfo.allowed))
			return MakeErrorPtr(CARD_UNOFFICIAL, kv.first);
		if(CheckPrelease(ced.scope, hostInfo.allowed))
			return MakeErrorPtr(CARD_UNOFFICIAL, kv.first);
		if(CheckOCG(ced.scope, hostInfo.allowed))
			return MakeErrorPtr(CARD_TCG_ONLY, kv.first);
		if(CheckTCG(ced.scope, hostInfo.allowed))
			return MakeErrorPtr(CARD_OCG_ONLY, kv.first);
		if(banlist && CheckBanlist(kv, *banlist))
			return MakeErrorPtr(CARD_BANLISTED, kv.first);
	}
	return nullptr;
}

} // namespace Ignis::Multirole::Room
