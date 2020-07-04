#include "../Context.hpp"

#include <spdlog/spdlog.h>

#include "../../Core/IScriptSupplier.hpp"
#include "../../YGOPro/Constants.hpp"
#include "../../YGOPro/CoreUtils.hpp"

#define CORE_EXCEPTION_HANDLER() catch(...) { throw; } // TODO

namespace Ignis::Multirole::Room
{

StateOpt Context::operator()(State::Dueling& s)
{
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
	X(EXTRA_RULE_SEALED_DUEL,        511005092U); // NOLINT
	X(EXTRA_RULE_BOOSTER_DUEL,       511005093U); // NOLINT
	X(EXTRA_RULE_DESTINY_DRAW,       511004000U); // NOLINT
	X(EXTRA_RULE_CONCENTRATION_DUEL, 511004322U); // NOLINT
	X(EXTRA_RULE_BOSS_DUEL,          95000000U);  // NOLINT
	X(EXTRA_RULE_BATTLE_CITY,        511004014U); // NOLINT
	X(EXTRA_RULE_DUELIST_KINGDOM,    511002621U); // NOLINT
	X(EXTRA_RULE_DIMENSION_DUEL,     511600002U); // NOLINT
	X(EXTRA_RULE_TURBO_DUEL,         110000000U); // NOLINT
	X(EXTRA_RULE_COMMAND_DUEL,       95200000U);  // NOLINT
	X(EXTRA_RULE_DECK_MASTER,        300U);       // NOLINT
	X(EXTRA_RULE_ACTION_DUEL,        151999999U); // NOLINT
#undef X
	OCG_NewCardInfo nci{};
	try
	{
		nci.pos = POS_FACEDOWN_DEFENSE;
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
		if(hostInfo.dontShuffleDeck != 0U)
			std::reverse(deck.begin(), deck.end());
		else
			std::shuffle(deck.begin(), deck.end(), *rng);
		return deck;
	};
	try
	{
		for(const auto& kv : duelists)
		{
			const auto& deck = *kv.second->CurrentDeck();
			nci.team = nci.con = GetSwappedTeam(kv.first.first);
			nci.duelist = kv.first.second;
			nci.loc = LOCATION_DECK;
			for(auto code : ReversedOrShuffled(deck.Main()))
			{
				nci.code = code;
				core.AddCard(s.duelPtr, nci);
			}
			nci.loc = LOCATION_EXTRA;
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
				core.QueryCount(s.duelPtr, 0U, LOCATION_DECK),
				core.QueryCount(s.duelPtr, 0U, LOCATION_EXTRA),
				core.QueryCount(s.duelPtr, 1U, LOCATION_DECK),
				core.QueryCount(s.duelPtr, 1U, LOCATION_EXTRA),
			});
		SendToTeam(GetSwappedTeam(0U), MakeGameMsg(msgStart));
		msgStart[1] = 1U;
		SendToTeam(GetSwappedTeam(1U), MakeGameMsg(msgStart));
		msgStart[1] = 0xF0 | isTeam1GoingFirst;
		SendToSpectators(MakeGameMsg(msgStart));
		// Update replay with deck data.
		auto RecordDecks = [&](uint8_t team)
		{
			const Core::IWrapper::QueryInfo qInfo =
			{
				0x1181FFF,
				team,
				LOCATION_DECK,
				0U,
				0U
			};
			const auto buffer = core.QueryLocation(s.duelPtr, qInfo);
			// TODO: Save into replay.
		};
		RecordDecks(0U);
		RecordDecks(1U);
		// Send extra deck queries.
		auto SendExtraDecks = [&](uint8_t team)
		{
			const Core::IWrapper::QueryInfo qInfo =
			{
				0x381FFF,
				team,
				LOCATION_EXTRA,
				0U,
				0U
			};
			const auto buffer = core.QueryLocation(s.duelPtr, qInfo);
			using namespace YGOPro::CoreUtils;
			SendToTeam(GetSwappedTeam(qInfo.con),
				MakeGameMsg(MakeUpdateDataMsg(qInfo.con, qInfo.loc, buffer)));
		};
		SendExtraDecks(0U);
		SendExtraDecks(1U);
	}
	CORE_EXCEPTION_HANDLER()
	// Start processing the duel
	if(const auto dfrOpt = Process(s); dfrOpt)
		return Finish(s, *dfrOpt);
	return std::nullopt;
}

StateOpt Context::operator()(State::Dueling& s, const Event::ConnectionLost& e)
{
	using Reason = DuelFinishReason::Reason;
	const auto p = e.client.Position();
	if(p == Client::POSITION_SPECTATOR)
	{
		e.client.Disconnect();
		return std::nullopt;
	}
	uint8_t winner = 1U - p.first;
	return Finish(s, DuelFinishReason{Reason::REASON_CONNECTION_LOST, winner});
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
	if(const auto dfrOpt = Process(s); dfrOpt)
		return Finish(s, *dfrOpt);
	return std::nullopt;
}

StateOpt Context::operator()(State::Dueling& s, const Event::Surrender& e)
{
	using Reason = DuelFinishReason::Reason;
	const auto p = e.client.Position();
	if(p == Client::POSITION_SPECTATOR)
	{
		e.client.Disconnect();
		return std::nullopt;
	}
	uint8_t winner = 1U - p.first;
	return Finish(s, DuelFinishReason{Reason::REASON_SURRENDERED, winner});
}

// private

Client& Context::GetCurrentTeamClient(State::Dueling& s, uint8_t team)
{
	return *duelists[{team, s.currentPos[team]}];
}

std::optional<Context::DuelFinishReason> Context::Process(State::Dueling& s)
{
	using namespace YGOPro::CoreUtils;
	auto& core = *cpkg.core;
	std::optional<DuelFinishReason> dfrOpt;
	auto AnalyzeMsg = [&](const Msg& msg)
	{
		using Reason = DuelFinishReason::Reason;
		uint8_t msgType = GetMessageType(msg);
		if(msgType == MSG_RETRY)
		{
			uint8_t winner = 1U - s.replier->Position().first;
			dfrOpt = DuelFinishReason{Reason::REASON_WRONG_RESPONSE, winner};
		}
		else if(msgType == MSG_WIN)
		{
			uint8_t winner = GetSwappedTeam(msg[1U]);
			dfrOpt = DuelFinishReason{Reason::REASON_DUEL_WON, winner};
		}
		else if(msgType == MSG_TAG_SWAP)
		{
			uint8_t team = GetSwappedTeam(msg[1U]);
			s.currentPos[team] = (s.currentPos[team] + 1U) %
				((team == 0U) ? hostInfo.t0Count : hostInfo.t1Count);
		}
		else if(msgType == MSG_MATCH_KILL)
		{
			// Too lazy to create a function in YGOPro::CoreUtils
			uint32_t reason{};
			std::memcpy(&reason, &msg[1U], sizeof(decltype(reason)));
			s.matchKillReason = reason;
		}
		else if(DoesMessageRequireAnswer(msgType))
		{
			uint8_t team = GetMessageReceivingTeam(msg);
			s.replier = &GetCurrentTeamClient(s, GetSwappedTeam(team));
			SendToAllExcept(*s.replier, MakeGameMsg({MSG_WAITING}));
			// TODO: update timers
		}
	};
	auto ProcessQueryRequests = [&](const std::vector<QueryRequest>& qreqs)
	{
		for(const auto& reqVar : qreqs)
		{
			if(std::holds_alternative<QuerySingleRequest>(reqVar))
			{
				const auto& req = std::get<QuerySingleRequest>(reqVar);
				auto MakeMsg = [&](const QueryBuffer& qb) -> YGOPro::STOCMsg
				{
					return MakeGameMsg(
						MakeUpdateCardMsg(req.con, req.loc, req.seq, qb));
				};
				const Core::IWrapper::QueryInfo qInfo =
				{
					req.flags,
					req.con,
					req.loc,
					req.seq,
					0U
				};
				const auto fullBuffer = core.Query(s.duelPtr, qInfo);
				const auto query = DeserializeSingleQueryBuffer(fullBuffer);
				const auto ownerBuffer = SerializeSingleQuery(query, false);
				const auto strippedBuffer = SerializeSingleQuery(query, true);
				const auto strippedMsg = MakeMsg(strippedBuffer);
				uint8_t team = GetSwappedTeam(req.con);
				SendToTeam(team, MakeMsg(ownerBuffer));
				SendToTeam(1U - team, strippedMsg);
				SendToSpectators(strippedMsg);
			}
			else /*if(std::holds_alternative<QueryLocationRequest>(reqVar))*/
			{
				const auto& req = std::get<QueryLocationRequest>(reqVar);
				auto MakeMsg = [&](const QueryBuffer& qb) -> YGOPro::STOCMsg
				{
					return MakeGameMsg(
						MakeUpdateDataMsg(req.con, req.loc, qb));
				};
				const Core::IWrapper::QueryInfo qInfo =
				{
					req.flags,
					req.con,
					req.loc,
					0U,
					0U
				};
				uint8_t team = GetSwappedTeam(req.con);
				const auto fullBuffer = core.QueryLocation(s.duelPtr, qInfo);
				if(req.loc == LOCATION_DECK)
				{
					// TODO: Save into replay
					continue;
				}
				else if(req.loc == LOCATION_EXTRA)
				{
					SendToTeam(team, MakeMsg(fullBuffer));
					continue;
				}
				const auto query = DeserializeLocationQueryBuffer(fullBuffer);
				const auto ownerBuffer = SerializeLocationQuery(query, false);
				const auto strippedBuffer = SerializeLocationQuery(query, true);
				const auto strippedMsg = MakeMsg(strippedBuffer);
				SendToTeam(team, MakeMsg(ownerBuffer));
				SendToTeam(1U - team, strippedMsg);
				SendToSpectators(strippedMsg);
			}
		}
	};
	auto DistributeMsg = [&](const Msg& msg)
	{
		switch(GetMessageDistributionType(msg))
		{
		case MsgDistType::MSG_DIST_TYPE_SPECIFIC_TEAM_DUELIST_STRIPPED:
		{
			uint8_t team = GetMessageReceivingTeam(msg);
			const auto sMsg = StripMessageForTeam(team, msg);
			s.replier->Send(MakeGameMsg(sMsg));
			break;
		}
		case MsgDistType::MSG_DIST_TYPE_SPECIFIC_TEAM_DUELIST:
		{
			s.replier->Send(MakeGameMsg(msg));
			break;
		}
		case MsgDistType::MSG_DIST_TYPE_SPECIFIC_TEAM:
		{
			uint8_t team = GetMessageReceivingTeam(msg);
			SendToTeam(GetSwappedTeam(team), MakeGameMsg(msg));
			break;
		}
		case MsgDistType::MSG_DIST_TYPE_EVERYONE_EXCEPT_TEAM_DUELIST:
		{
			uint8_t team = GetMessageReceivingTeam(msg);
			SendToAllExcept(
				GetCurrentTeamClient(s, GetSwappedTeam(team)),
				MakeGameMsg(msg));
			break;
		}
		case MsgDistType::MSG_DIST_TYPE_EVERYONE_STRIPPED:
		{
			const std::array<Msg, 2U> sMsgs =
			{
				StripMessageForTeam(0U, msg),
				StripMessageForTeam(1U, msg)
			};
			SendToTeam(GetSwappedTeam(0U), MakeGameMsg(sMsgs[0U]));
			SendToTeam(GetSwappedTeam(1U), MakeGameMsg(sMsgs[1U]));
			SendToSpectators(MakeGameMsg(StripMessageForTeam(1U, sMsgs[0U])));
			break;
		}
		case MsgDistType::MSG_DIST_TYPE_EVERYONE:
		{
			SendToAll(MakeGameMsg(msg));
			break;
		}
		}
	};
	auto ProcessSingleMsg = [&](const Msg& msg)
	{
		AnalyzeMsg(msg);
		ProcessQueryRequests(GetPreDistQueryRequests(msg));
		DistributeMsg(msg);
		ProcessQueryRequests(GetPostDistQueryRequests(msg));
	};
	try
	{
		for(;;)
		{
			Core::IWrapper::DuelStatus status = core.Process(s.duelPtr);
			for(const auto& msg : SplitToMsgs(core.GetMessages(s.duelPtr)))
			{
				ProcessSingleMsg(msg);
				if(dfrOpt) // Possibly set by AnalyzeMsg
					return dfrOpt;
			}
			if(status != Core::IWrapper::DUEL_STATUS_CONTINUE)
				break;
		}
	}
	CORE_EXCEPTION_HANDLER()
	return dfrOpt;
}

StateVariant Context::Finish(State::Dueling& s, const DuelFinishReason& dfr)
{
	using Reason = DuelFinishReason::Reason;
	if(dfr.reason != Reason::REASON_CORE_CRASHED)
	{
		// NOTE: if the core crashes here, it'll be handled on next duel
		// creation. Or if the room lifetime ended, it wouldn't matter anyway
		// as the crash frees all the memory for us.
		try
		{
			cpkg.core->DestroyDuel(s.duelPtr);
		}
		catch(...){}
	}
	auto SendWinMsg = [&](uint8_t reason)
	{
		SendToAll(MakeGameMsg({MSG_WIN, GetSwappedTeam(dfr.winner), reason}));
	};
	auto turnDecider = [&]() -> Client*
	{
		if(dfr.winner <= 1U)
			return duelists[{1U - dfr.winner, 0U}];
		return duelists[{0U, 0U}];
	}();
	switch(dfr.reason)
	{
	case Reason::REASON_DUEL_WON:
	case Reason::REASON_SURRENDERED:
	case Reason::REASON_TIMED_OUT:
	case Reason::REASON_WRONG_RESPONSE:
	{
		// Send corresponding game finishing messages
		if(dfr.reason == Reason::REASON_SURRENDERED)
			SendWinMsg(WIN_REASON_SURRENDERED);
		else if(dfr.reason == Reason::REASON_TIMED_OUT)
			SendWinMsg(WIN_REASON_TIMED_OUT);
		else if(dfr.reason == Reason::REASON_WRONG_RESPONSE)
			SendWinMsg(WIN_REASON_WRONG_RESPONSE);
		if(hostInfo.bestOf <= 1 || dfr.winner == 2U)
			return State::Rematching{turnDecider, {}};
		wins[dfr.winner] += (s.matchKillReason) ? hostInfo.bestOf : 1U;
		if(wins[dfr.winner] >= neededWins)
		{
			SendToAll(MakeDuelEnd());
			return State::Closing{};
		}
		return State::Sidedecking{turnDecider, {}};
	}
	case Reason::REASON_CONNECTION_LOST:
	{
		SendWinMsg(WIN_REASON_CONNECTION_LOST);
		SendToAll(MakeDuelEnd());
		return State::Closing{};
	}
	case Reason::REASON_CORE_CRASHED:
	{
		SendToAll(MakeChat(CHAT_MSG_TYPE_ERROR, "Core crashed!"));
		[[fallthrough]];
	}
	default:
		SendToAll(MakeDuelEnd());
		return State::Closing{};
	}
}

} // namespace Ignis::Multirole::Room
