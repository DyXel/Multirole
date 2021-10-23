#include "Context.hpp"

#include "../I18N.hpp"
#include "../STOCMsgFactory.hpp"
#include "../Service/DataProvider.hpp"
#include "../Service/LogHandler.hpp"
#include "../YGOPro/Banlist.hpp"
#include "../YGOPro/CardDatabase.hpp"
#include "../YGOPro/Constants.hpp"
#include "../YGOPro/Deck.hpp"

namespace Ignis::Multirole::Room
{

Context::Context(CreateInfo&& info) noexcept
	:
	STOCMsgFactory(info.hostInfo.t0Count),
	svc(info.svc),
	tagg(info.tagg),
	id(info.id),
	banlist(std::move(info.banlist)),
	hostInfo(info.hostInfo),
	limits(info.limits),
	cdb(svc.dataProvider.GetDatabase()),
	neededWins((hostInfo.bestOf / 2) + (hostInfo.bestOf & 1)),
	joinMsg(YGOPro::STOCMsg::JoinGame{hostInfo}),
	isPrivate(info.isPrivate),
	rl(svc.logHandler.MakeRoomLogger(id)),
	scriptLogger(svc.logHandler, hostInfo),
	rng(info.seed)
{
	if(rl)
		rl->Log(I18N::ROOM_LOGGER_ROOM_NOTES, info.notes);
}

Context::~Context() noexcept = default;

const YGOPro::HostInfo& Context::HostInfo() const noexcept
{
	return hostInfo;
}

bool Context::IsPrivate() const noexcept
{
	return isPrivate;
}

DuelistsMap Context::GetDuelistsNames() const noexcept
{
	DuelistsMap ret{0U, {}};
	std::shared_lock lock(mDuelists);
	for(const auto& kv : duelists)
	{
		if(ret.usedCount >= ret.pairs.size())
			break; // We can't store the rest of names.
		const auto& strName = kv.second->Name();
		auto& duelist = ret.pairs[ret.usedCount];
		duelist.pos = EncodePosition(kv.first);
		duelist.nameLength = std::min(duelist.name.size(), strName.size());
		std::copy_n(strName.cbegin(), duelist.nameLength, duelist.name.begin());
		duelist.name.back() = '\0'; // NOTE: Let's be extra safe.
		ret.usedCount++;
	}
	return ret;
}

// private

bool Context::IsTiebreaking() const noexcept
{
	// FIXME: Actually read hostInfo when client is updated to handle it.
	return true;
}

uint8_t Context::GetSwappedTeam(uint8_t team) const noexcept
{
	assert(team <= 1U);
	return isTeam1GoingFirst ^ team;
}

std::array<uint8_t, 2U> Context::GetTeamCounts() const noexcept
{
	std::array<uint8_t, 2U> ret{};
	for(const auto& kv : duelists)
		ret[kv.first.first]++;
	return ret;
}

void Context::SendToTeam(uint8_t team, const YGOPro::STOCMsg& msg) noexcept
{
	assert(team <= 1U);
	for(const auto& kv : duelists)
	{
		if(kv.first.first != team)
			continue;
		kv.second->Send(msg);
	}
}

void Context::SendToSpectators(const YGOPro::STOCMsg& msg) noexcept
{
	for(const auto& c : spectators)
		c->Send(msg);
}

void Context::SendToAll(const YGOPro::STOCMsg& msg) noexcept
{
	for(const auto& kv : duelists)
		kv.second->Send(msg);
	SendToSpectators(msg);
}

void Context::SendToAllExcept(Client& client, const YGOPro::STOCMsg& msg) noexcept
{
	for(const auto& kv : duelists)
	{
		if(kv.second == &client)
			continue;
		kv.second->Send(msg);
	}
	SendToSpectators(msg);
}

void Context::SendDuelistsInfo(Client& client) noexcept
{
	for(const auto& kv : duelists)
	{
		if(kv.second == &client)
			continue; // Skip itself
		client.Send(MakePlayerEnter(*kv.second));
		client.Send(MakePlayerChange(*kv.second));
	}
}

void Context::SetupAsSpectator(Client& client, bool sendJoin) noexcept
{
	spectators.insert(&client);
	client.SetPosition(Client::POSITION_SPECTATOR);
	if(sendJoin)
		client.Send(joinMsg);
	client.Send(MakeTypeChange(client, false));
	SendDuelistsInfo(client);
}

void Context::MakeAndSendChat(Client& client, std::string_view msg) noexcept
{
	if(auto p = client.Position(); p == Client::POSITION_SPECTATOR)
	{
		SendToAll(MakeChat(client, false, msg));
	}
	else
	{
		SendToTeam(client.Position().first, MakeChat(client, true, msg));
		const auto stocMsg = MakeChat(client, false, msg);
		SendToTeam(1U - client.Position().first, stocMsg);
		SendToSpectators(stocMsg);
	}
	if(!isPrivate && rl)
		rl->Log(I18N::ROOM_LOGGER_CHAT, client.Name(), client.Ip(), msg);
}

std::unique_ptr<YGOPro::Deck> Context::LoadDeck(
	const std::vector<uint32_t>& main,
	const std::vector<uint32_t>& side) const noexcept
{
	auto IsExtraDeckCardType = [](uint32_t type) constexpr -> bool
	{
		if((type & (TYPE_FUSION | TYPE_SYNCHRO | TYPE_XYZ)) != 0U)
			return true;
		// NOTE: Link Spells exist.
		if(((type & TYPE_LINK) != 0U) && ((type & TYPE_MONSTER) != 0U))
			return true;
		return false;
	};
	YGOPro::CodeVector m;
	YGOPro::CodeVector e;
	YGOPro::CodeVector s;
	uint32_t err = 0U;
	for(const auto code : main)
	{
		const auto& data = cdb->DataFromCode(code);
		if(data.code == 0U)
		{
			err = code;
			continue;
		}
		if((data.type & TYPE_TOKEN) != 0U)
			continue;
		if(IsExtraDeckCardType(data.type))
			e.push_back(code);
		else
			m.push_back(code);
	}
	for(const auto code : side)
	{
		const auto& data = cdb->DataFromCode(code);
		if(data.code == 0U)
		{
			err = code;
			continue;
		}
		if((data.type & TYPE_TOKEN) != 0U)
			continue;
		s.push_back(code);
	}
	return std::make_unique<YGOPro::Deck>(
		std::move(m),
		std::move(e),
		std::move(s),
		err);
}

std::unique_ptr<YGOPro::STOCMsg> Context::CheckDeck(const YGOPro::Deck& deck) const noexcept
{
	using namespace Error;
	using namespace YGOPro;
	// Handy shortcuts.
	auto MakeErrorPtr = [](DeckOrCard type, uint32_t value)
	{
		return std::make_unique<STOCMsg>(MakeDeckError(type, value));
	};
	auto MakeErrorLimitsPtr = [](DeckOrCard type, std::size_t got, const auto& lim)
	{
		return std::make_unique<STOCMsg>(
			MakeDeckError(type, got, lim.min, lim.max));
	};
	// Check if the deck had any error while loading.
	if(const auto error = deck.Error(); error != 0U)
		return MakeErrorPtr(CARD_UNKNOWN, error);
	// Check if the deck obeys the limits.
	auto OutOfBound = [](const auto& lim, const CodeVector& vector) -> auto
	{
		std::pair<std::size_t, bool> p{vector.size(), false};
		return (p.second = p.first < lim.min || p.first > lim.max), p;
	};
	if(const auto p = OutOfBound(limits.main, deck.Main()); p.second)
		return MakeErrorLimitsPtr(DECK_BAD_MAIN_COUNT, p.first, limits.main);
	if(const auto p = OutOfBound(limits.extra, deck.Extra()); p.second)
		return MakeErrorLimitsPtr(DECK_BAD_EXTRA_COUNT, p.first, limits.extra);
	if(const auto p = OutOfBound(limits.side, deck.Side()); p.second)
		return MakeErrorLimitsPtr(DECK_BAD_SIDE_COUNT, p.first, limits.side);
	// Check per-code properties.
	// Get un-aliased map that will be updated a bit later...
	auto aliased = deck.GetCodeMap();
	// Make copy of "un-aliased" card codes.
	const auto codes = [&aliased]()
	{
		std::set<uint32_t> ret;
		for(const auto& kv : aliased)
			ret.insert(kv.first);
		return ret;
	}();
	// Save alias mapping while updating map to only store aliased counts.
	const auto aliases = [&]()
	{
		std::map<uint32_t, uint32_t> ret;
		for(auto it = aliased.begin(), last = aliased.end(); it != last;)
		{
			uint32_t code = it->first;
			if(uint32_t alias = cdb->DataFromCode(code).alias; alias != 0U)
			{
				ret[code] = alias;
				aliased[alias] = aliased[alias] + it->second;
				it = aliased.erase(it);
			}
			else
			{
				++it;
			}
		}
		return ret;
	}();
	// Fetch actual count of particular card even if aliased.
	auto GetTotalCount = [&aliased, &aliases](uint32_t code) -> std::size_t
	{
		if(auto search = aliases.find(code); search != aliases.end())
			return aliased[search->second];
		return aliased[code];
	};
	// Custom predicates...
	//	true if card scope is unnofficial and not allowed.
	auto CheckUnofficial = [](uint32_t scope, uint8_t allowed) constexpr -> bool
	{
		switch(allowed)
		{
		case ALLOWED_CARDS_OCG_ONLY:
		case ALLOWED_CARDS_TCG_ONLY:
		case ALLOWED_CARDS_OCG_TCG:
			return scope > SCOPE_OCG_TCG;
		case ALLOWED_CARDS_WITH_PRERELEASE:
			return (scope & (~SCOPE_OFFICIAL)) != 0U;
		default:
			return false;
		}
	};
	//	true if card scope is prerelease and they are not allowed.
	auto CheckPrelease = [](uint32_t scope, uint8_t allowed) constexpr -> bool
	{
		return allowed == ALLOWED_CARDS_WITH_PRERELEASE &&
		       ((scope & SCOPE_OFFICIAL) == 0U);
	};
	//	true if only ocg are allowed and scope is not ocg (its tcg).
	auto CheckOCG = [](uint32_t scope, uint8_t allowed) constexpr -> bool
	{
		return allowed == ALLOWED_CARDS_OCG_ONLY && ((scope & SCOPE_OCG) == 0U);
	};
	//	true if only tcg are allowed and scope is not tcg (its ocg).
	auto CheckTCG = [](uint32_t scope, uint8_t allowed) constexpr -> bool
	{
		return allowed == ALLOWED_CARDS_TCG_ONLY && ((scope & SCOPE_TCG) == 0U);
	};
	//	true if card code and its count is banlisted.
	auto CheckBanlist = [&](uint32_t code, std::size_t count, const Banlist& bl) -> bool
	{
		const auto& d = bl.Dict();
		const auto eit = d.end();
		auto it = d.find(code);
		if(it == eit)
			if(auto search = aliases.find(code); search != aliases.end())
				it = d.find(search->second);
		return ((it != eit) && static_cast<int32_t>(count) > it->second) ||
		       ((it == eit) && bl.IsWhitelist());
	};
	for(const auto code : codes)
	{
		const uint32_t totalCount = GetTotalCount(code);
		if(totalCount > 3U)
			return MakeErrorPtr(CARD_MORE_THAN_3, code);
		if((cdb->DataFromCode(code).type & hostInfo.forb) != 0U)
			return MakeErrorPtr(CARD_FORBIDDEN_TYPE, code);
		const auto& ced = cdb->ExtraFromCode(code);
		if(CheckUnofficial(ced.scope, hostInfo.allowed))
			return MakeErrorPtr(CARD_UNOFFICIAL, code);
		if(CheckPrelease(ced.scope, hostInfo.allowed))
			return MakeErrorPtr(CARD_UNOFFICIAL, code);
		if(CheckOCG(ced.scope, hostInfo.allowed))
			return MakeErrorPtr(CARD_TCG_ONLY, code);
		if(CheckTCG(ced.scope, hostInfo.allowed))
			return MakeErrorPtr(CARD_OCG_ONLY, code);
		if((banlist != nullptr) && CheckBanlist(code, totalCount, *banlist))
			return MakeErrorPtr(CARD_BANLISTED, code);
	}
	return nullptr;
}

} // namespace Ignis::Multirole::Room
