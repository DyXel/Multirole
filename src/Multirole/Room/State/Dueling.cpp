#include "../Context.hpp"

#include <spdlog/spdlog.h>

#include "../../Core/IScriptSupplier.hpp"
#include "../../YGOPro/CoreUtils.hpp"

#define CORE_EXCEPTION_HANDLER() catch(...) { throw; } // TODO

namespace Ignis::Multirole::Room
{

constexpr uint8_t GetSwappedTeam(const State::Dueling& s, uint8_t team)
{
	assert(team <= 1u);
	return s.isTeam1GoingFirst ^ team;
}

void Context::operator()(State::Dueling& s)
{
	// TODO: Set up Replay
	using namespace YGOPro;
	// The RNG is lazily initialized here because most rooms would have not
	// reached this point and allocating it when the room is created
	// would have been a waste.
	if(!rng)
		rng = std::make_unique<std::mt19937>(id);
	// Create core duel with room's options
	auto& core = *cpkg.core;
	const OCG_Player popt =
	{
		static_cast<int>(hostInfo.startingLP),
		hostInfo.startingDrawCount,
		hostInfo.drawCountPerTurn
	};
	const Core::IWrapper::DuelOptions dopts =
	{
		static_cast<uint32_t>((*rng)()),
		static_cast<int>(hostInfo.duelFlags),
		popt,
		popt
	};
	try
	{
		s.duelPtr = core.CreateDuel(dopts);
		auto& supplier = *core.GetScriptSupplier();
		auto LoadScript = [&](std::string_view file)
		{
			if(auto scr = supplier.ScriptFromFilePath(file); !scr.empty())
				core.LoadScript(s.duelPtr, file, scr);
		};
		LoadScript("constant.lua");
		LoadScript("utility.lua");
	}
	CORE_EXCEPTION_HANDLER()
	// Enable extra rules for the duel.
	// These are controlled simply by adding custom cards
	// with the rulesets to the game as playable cards, they will
	// trigger immediately after the duel starts.
	std::vector<uint32_t> extraCards;
#define X(f, c) if(hostInfo.extraRules & (f)) extraCards.push_back(c)
	// NOTE: no lint used because we dont want clang-tidy to complain
	// about magic numbers we already know.
	X(EXTRA_RULE_SEALED_DUEL,        511005092); // NOLINT
	X(EXTRA_RULE_BOOSTER_DUEL,       511005093); // NOLINT
	X(EXTRA_RULE_DESTINY_DRAW,       511004000); // NOLINT
	X(EXTRA_RULE_CONCENTRATION_DUEL, 511004322); // NOLINT
	X(EXTRA_RULE_BOSS_DUEL,          95000000);  // NOLINT
	X(EXTRA_RULE_BATTLE_CITY,        511004014); // NOLINT
	X(EXTRA_RULE_DUELIST_KINGDOM,    511002621); // NOLINT
	X(EXTRA_RULE_DIMENSION_DUEL,     511600002); // NOLINT
	X(EXTRA_RULE_TURBO_DUEL,         110000000); // NOLINT
	X(EXTRA_RULE_COMMAND_DUEL,       95200000);  // NOLINT
	X(EXTRA_RULE_DECK_MASTER,        300);       // NOLINT
	X(EXTRA_RULE_ACTION_DUEL,        151999999); // NOLINT
#undef X
	OCG_NewCardInfo nci{};
	try
	{
		nci.pos = 0x8; // POS_FACEDOWN_DEFENSE
		for(auto code : extraCards)
		{
			nci.code = code;
			core.AddCard(s.duelPtr, nci);
		}
	}
	CORE_EXCEPTION_HANDLER()
	// Add main and extra deck cards for all players.
	auto ReversedOrShuffled = [&](CodeVector deck) // NOTE: Copy is intentional.
	{
		if(hostInfo.dontShuffleDeck != 0u)
			std::reverse(deck.begin(), deck.end());
		else
			std::shuffle(deck.begin(), deck.end(), *rng);
		return deck;
	};
	try
	{
		for(const auto& kv : duelists)
		{
			auto& deck = *kv.second->CurrentDeck();
			nci.team = nci.con = kv.first.first;
			nci.duelist = kv.first.second;
			nci.loc = 0x01; // LOCATION_DECK
			for(auto code : ReversedOrShuffled(deck.Main()))
			{
				nci.code = code;
				core.AddCard(s.duelPtr, nci);
			}
			nci.loc = 0x40; // LOCATION_EXTRA
			for(auto code : deck.Extra())
			{
				nci.code = code;
				core.AddCard(s.duelPtr, nci);
			}
		}
		core.Start(s.duelPtr);
		// Send MSG_START message to clients.
		auto msgStart = CoreUtils::MakeStartMsg(
			{
				hostInfo.startingLP,
				core.QueryCount(s.duelPtr, 0, 0x01), // LOCATION_DECK
				core.QueryCount(s.duelPtr, 0, 0x40), // LOCATION_EXTRA
				core.QueryCount(s.duelPtr, 1, 0x01), // LOCATION_DECK
				core.QueryCount(s.duelPtr, 1, 0x40), // LOCATION_EXTRA
			});
		SendToTeam(GetSwappedTeam(s, 0), CoreUtils::GameMsgFromMsg(msgStart));
		msgStart[1] = 1;
		SendToTeam(GetSwappedTeam(s, 1), CoreUtils::GameMsgFromMsg(msgStart));
		msgStart[1] = 0xF0; // NOLINT: Magic number for spectators.
		SendToSpectators(CoreUtils::GameMsgFromMsg(msgStart));
	}
	CORE_EXCEPTION_HANDLER()
	// Start processing the duel
	Process(s);
}

StateOpt Context::operator()(State::Dueling& s, const Event::Response& e)
{
	if(s.replier != &e.client)
		return std::nullopt;
	try
	{
		cpkg.core->SetResponse(s.duelPtr, e.data);
	}
	CORE_EXCEPTION_HANDLER()
	Process(s);
	return std::nullopt;
}

// private

Client& Context::GetCurrentTeamClient(State::Dueling& s, uint8_t team)
{
	return *duelists[{team, s.currentPos[team]}];
}

void Context::Process(State::Dueling& s)
{
	using namespace YGOPro::CoreUtils;
	auto& core = *cpkg.core;
	auto DistributeMsg = [&](const Msg& msg)
	{
		switch(GetMessageDistributionType(msg))
		{
		case MsgDistType::MSG_DIST_TYPE_SPECIFIC_TEAM_DUELIST_STRIPPED:
		{
			uint8_t team = GetMessageReceivingTeam(msg);
			const auto sMsg = StripMessageForTeam(team, msg);
			s.replier = &GetCurrentTeamClient(s, GetSwappedTeam(s, team));
			s.replier->Send(GameMsgFromMsg(sMsg));
			break;
		}
		case MsgDistType::MSG_DIST_TYPE_SPECIFIC_TEAM_DUELIST:
		{
			uint8_t team = GetMessageReceivingTeam(msg);
			s.replier = &GetCurrentTeamClient(s, GetSwappedTeam(s, team));
			s.replier->Send(GameMsgFromMsg(msg));
			break;
		}
		case MsgDistType::MSG_DIST_TYPE_SPECIFIC_TEAM:
		{
			uint8_t team = GetMessageReceivingTeam(msg);
			SendToTeam(GetSwappedTeam(s, team), GameMsgFromMsg(msg));
			break;
		}
		case MsgDistType::MSG_DIST_TYPE_EVERYONE_EXCEPT_TEAM_DUELIST:
		{
			uint8_t team = GetMessageReceivingTeam(msg);
			SendToAllExcept(
				GetCurrentTeamClient(s, GetSwappedTeam(s, team)),
				GameMsgFromMsg(msg));
			break;
		}
		case MsgDistType::MSG_DIST_TYPE_EVERYONE_STRIPPED:
		{
			const std::array<Msg, 2> sMsgs =
			{
				StripMessageForTeam(0, msg),
				StripMessageForTeam(1, msg)
			};
			SendToTeam(GetSwappedTeam(s, 0), GameMsgFromMsg(sMsgs[0]));
			SendToTeam(GetSwappedTeam(s, 1), GameMsgFromMsg(sMsgs[1]));
			SendToSpectators(GameMsgFromMsg(StripMessageForTeam(1, sMsgs[0])));
			break;
		}
		case MsgDistType::MSG_DIST_TYPE_EVERYONE:
		{
			SendToAll(GameMsgFromMsg(msg));
			break;
		}
		}
	};
	auto ProcessSingleMsg = [&](const Msg& msg)
	{
		spdlog::info("Processing = {}", GetMessageType(msg));
		// TODO: pre queries here
		DistributeMsg(msg);
		// TODO: analyze here
		// TODO: post queries here
		// TODO: add to replay
	};
	try
	{
		Core::IWrapper::DuelStatus status;
		do
		{
			status = core.Process(s.duelPtr);
			spdlog::info("status = {}", status);
			for(const auto& msg : SplitToMsgs(core.GetMessages(s.duelPtr)))
				ProcessSingleMsg(msg);
		}while(status == Core::IWrapper::DUEL_STATUS_CONTINUE);
	}
	catch(const std::out_of_range& e)
	{
		spdlog::error("Process: {}", e.what());
	}
	CORE_EXCEPTION_HANDLER()
}

} // namespace Ignis::Multirole::Room
