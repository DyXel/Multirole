#include "../Context.hpp"

#include "../TimerAggregator.hpp"
#include "../../I18N.hpp"
#include "../../Core/IWrapper.hpp"
#include "../../Service/LogHandler.hpp"
#define LOG_INFO(...) svc.logHandler.Log(ErrorCategory::CORE, s.replayId, __VA_ARGS__)
#define LOG_ERROR(...) svc.logHandler.Log(ErrorCategory::CORE, s.replayId, __VA_ARGS__)
#include "../../Service/ReplayManager.hpp"
#include "../../Service/ScriptProvider.hpp"
#include "../../YGOPro/CardDatabase.hpp"
#include "../../YGOPro/Constants.hpp"
#include "../../YGOPro/CoreUtils.hpp"

namespace Ignis::Multirole::Room
{

namespace
{

constexpr auto GRACE_PERIOD = std::chrono::seconds(5);

inline void ResetTimers(State::Dueling& s, uint32_t limitInSeconds) noexcept
{
	using namespace std::chrono;
	const auto secs = seconds(limitInSeconds) + GRACE_PERIOD;
	const auto time = duration_cast<milliseconds>(secs);
	s.timeRemaining = {time, time};
};

constexpr auto CORE_EXC_REASON = Context::DuelFinishReason
{
	Context::DuelFinishReason::Reason::REASON_CORE_CRASHED,
	2U // NOLINT: Draw.
};

} // namespace

StateOpt Context::operator()(State::Dueling& s) noexcept
{
	using namespace YGOPro;
	const auto seed = static_cast<uint32_t>(rng());
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
	X(EXTRA_RULE_DECK_MASTER,        153999999U); // NOLINT
	X(EXTRA_RULE_ACTION_DUEL,        151999999U); // NOLINT
#undef X
	// Construct replay.
	scriptLogger.SetReplayID(s.replayId = svc.replayManager.NewId());
	auto CurrentTime = []()
	{
		using namespace std::chrono;
		return system_clock::to_time_t(system_clock::now());
	};
	s.replay = std::make_unique<YGOPro::Replay>
	(
		static_cast<uint32_t>(CurrentTime()),
		seed,
		hostInfo,
		extraCards
	);
	// Create core duel with room's options.
	const OCG_Player popt =
	{
		hostInfo.startingLP,
		hostInfo.startingDrawCount,
		hostInfo.drawCountPerTurn
	};
	const Core::IWrapper::DuelOptions dopts =
	{
		*cdb,
		svc.scriptProvider,
		&scriptLogger,
		seed,
		HostInfo::OrDuelFlags(hostInfo.duelFlagsHigh, hostInfo.duelFlagsLow),
		popt,
		popt
	};
	try
	{
		s.duelPtr = s.core->CreateDuel(dopts);
		auto LoadScript = [&](std::string_view file)
		{
			if(auto scr = svc.scriptProvider.ScriptFromFilePath(file); !scr.empty())
				s.core->LoadScript(s.duelPtr, file, scr);
		};
		LoadScript("constant.lua");
		LoadScript("utility.lua");
	}
	catch(Core::Exception& e)
	{
		LOG_ERROR(I18N::ROOM_DUELING_CORE_EXCEPT_CREATION, e.what());
		return Finish(s, CORE_EXC_REASON);
	}
	OCG_NewCardInfo nci{};
	try
	{
		nci.pos = POS_FACEDOWN_DEFENSE;
		for(auto code : extraCards)
		{
			nci.code = code;
			s.core->AddCard(s.duelPtr, nci);
		}
	}
	catch(Core::Exception& e)
	{
		LOG_ERROR(I18N::ROOM_DUELING_CORE_EXCEPT_EXTRA_CARDS, e.what());
		return Finish(s, CORE_EXC_REASON);
	}
	// Add main and extra deck cards for all players.
	auto ReversedOrShuffled = [&](CodeVector deck) // NOTE: Copy is intentional.
	{
		if(hostInfo.dontShuffleDeck != 0U)
			std::reverse(deck.begin(), deck.end());
		else
			std::shuffle(deck.begin(), deck.end(), rng);
		return deck;
	};
	try
	{
		const auto teamCount = GetTeamCounts();
		for(const auto& kv : duelists)
		{
			const uint8_t t = kv.first.first;
			const auto& deck = *kv.second->CurrentDeck();
			nci.team = nci.con = GetSwappedTeam(t);
			nci.duelist = (kv.first.second + !!s.currentPos[t]) % teamCount[t];
			nci.loc = LOCATION_DECK;
			auto finalMainDeck = ReversedOrShuffled(deck.Main());
			for(auto code : finalMainDeck)
			{
				nci.code = code;
				s.core->AddCard(s.duelPtr, nci);
			}
			nci.loc = LOCATION_EXTRA;
			for(auto code : deck.Extra())
			{
				nci.code = code;
				s.core->AddCard(s.duelPtr, nci);
			}
			s.replay->AddDuelist(nci.team, nci.duelist,
			{
				duelists[{t, nci.duelist}]->Name(),
				std::move(finalMainDeck),
				deck.Extra()
			});
		}
		s.core->Start(s.duelPtr);
		// Create and send MSG_START message to clients.
		auto msgStart = CoreUtils::MakeStartMsg(
			{
				hostInfo.startingLP,
				s.core->QueryCount(s.duelPtr, 0U, LOCATION_DECK),
				s.core->QueryCount(s.duelPtr, 0U, LOCATION_EXTRA),
				s.core->QueryCount(s.duelPtr, 1U, LOCATION_DECK),
				s.core->QueryCount(s.duelPtr, 1U, LOCATION_EXTRA),
			});
		s.replay->RecordMsg(msgStart);
		SendToTeam(GetSwappedTeam(0U), MakeGameMsg(msgStart));
		msgStart[1] = 1U;
		SendToTeam(GetSwappedTeam(1U), MakeGameMsg(msgStart));
		msgStart[1] = 0xF0 | isTeam1GoingFirst;
		SendToSpectators(SaveToSpectatorCache(s, MakeGameMsg(msgStart)));
		// Record queries for deck data.
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
			using namespace YGOPro::CoreUtils;
			const auto buffer = s.core->QueryLocation(s.duelPtr, qInfo);
			s.replay->RecordMsg(MakeUpdateDataMsg(qInfo.con, qInfo.loc, buffer));
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
			using namespace YGOPro::CoreUtils;
			const auto buffer = s.core->QueryLocation(s.duelPtr, qInfo);
			const auto msg = MakeUpdateDataMsg(qInfo.con, qInfo.loc, buffer);
			s.replay->RecordMsg(msg);
			SendToTeam(GetSwappedTeam(qInfo.con), MakeGameMsg(msg));
		};
		SendExtraDecks(0U);
		SendExtraDecks(1U);
	}
	catch(Core::Exception& e)
	{
		LOG_ERROR(I18N::ROOM_DUELING_CORE_EXCEPT_STARTING, e.what());
		return Finish(s, CORE_EXC_REASON);
	}
	// Start processing the duel.
	ResetTimers(s, hostInfo.timeLimitInSeconds);
	if(const auto dfrOpt = Process(s); dfrOpt)
		return Finish(s, *dfrOpt);
	return std::nullopt;
}

StateOpt Context::operator()(State::Dueling& s, const Event::ConnectionLost& e) noexcept
{
	using Reason = DuelFinishReason::Reason;
	const auto p = e.client.Position();
	if(p == Client::POSITION_SPECTATOR)
	{
		spectators.erase(&e.client);
		return std::nullopt;
	}
	uint8_t winner = 1U - p.first;
	return Finish(s, DuelFinishReason{Reason::REASON_CONNECTION_LOST, winner});
}

StateOpt Context::operator()(State::Dueling& s, const Event::Join& e) noexcept
{
	SetupAsSpectator(e.client);
	e.client.Send(MakeDuelStart());
	e.client.Send(MakeCatchUp(true));
	for(const auto& msg : s.spectatorCache)
		e.client.Send(msg);
	e.client.Send(MakeCatchUp(false));
	return std::nullopt;
}

StateOpt Context::operator()(State::Dueling& s, const Event::Response& e) noexcept
{
	if(s.replier != &e.client)
		return std::nullopt;
	if(hostInfo.timeLimitInSeconds != 0U)
	{
		using namespace std::chrono;
		uint8_t team = e.client.Position().first;
		auto delta = tagg.Expiry(team) - system_clock::now();
		s.timeRemaining[team] = duration_cast<milliseconds>(ceil<seconds>(delta));
		tagg.Cancel(team);
	}
	s.replay->RecordResponse(e.data);
	try
	{
		s.core->SetResponse(s.duelPtr, e.data);
	}
	catch(Core::Exception& e)
	{
		LOG_ERROR(I18N::ROOM_DUELING_CORE_EXCEPT_RESPONSE, e.what());
		return Finish(s, CORE_EXC_REASON);
	}
	if(const auto dfrOpt = Process(s); dfrOpt)
		return Finish(s, *dfrOpt);
	return std::nullopt;
}

StateOpt Context::operator()(State::Dueling& s, const Event::Surrender& e) noexcept
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

StateOpt Context::operator()(State::Dueling& s, const Event::TimerExpired& e) noexcept
{
	using Reason = DuelFinishReason::Reason;
	uint8_t winner = 1U - e.team;
	return Finish(s, DuelFinishReason{Reason::REASON_TIMED_OUT, winner});
}

// private

Client& Context::GetCurrentTeamClient(State::Dueling& s, uint8_t team) noexcept
{
	return *duelists[{team, s.currentPos[team]}];
}

std::optional<Context::DuelFinishReason> Context::Process(State::Dueling& s) noexcept
{
	using namespace YGOPro::CoreUtils;
	auto PreAnalyzeMsg = [&](const Msg& msg) -> bool
	{
		uint8_t msgType = GetMessageType(msg);
		if(msgType == MSG_RETRY)
		{
			if(s.retryCount[s.replier->Position().first]++ < 1)
			{
				s.replier->Send(retryErrorMsg);
				if(!s.lastHint.empty())
					s.replier->Send(MakeGameMsg(s.lastHint));
				s.replier->Send(MakeGameMsg(s.lastRequest));
				s.replay->PopBackResponse();
				return false;
			}
		}
		else if(msgType == MSG_HINT && msg[1U] == 3U) // NOLINT: HINT_SELECTMSG
		{
			s.lastHint = msg;
		}
		else if(msgType == MSG_TAG_SWAP)
		{
			uint8_t team = GetSwappedTeam(msg[1U]);
			s.currentPos[team] = (s.currentPos[team] + 1U) %
				((team == 0U) ? hostInfo.t0Count : hostInfo.t1Count);
		}
		else if(msgType == MSG_MATCH_KILL)
		{
			// So lazy can't create separate function.
			uint32_t reason{};
			std::memcpy(&reason, &msg[1U], sizeof(decltype(reason)));
			s.matchKillReason = reason;
		}
		else if(msgType == MSG_NEW_TURN)
		{
			ResetTimers(s, hostInfo.timeLimitInSeconds);
		}
		else if(DoesMessageRequireAnswer(msgType))
		{
			uint8_t team = GetSwappedTeam(GetMessageReceivingTeam(msg));
			s.replier = &GetCurrentTeamClient(s, team);
			s.lastRequest = msg;
		}
		return true;
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
					return MakeGameMsg(MakeUpdateCardMsg(req.con, req.loc, req.seq, qb));
				};
				const Core::IWrapper::QueryInfo qInfo =
				{
					req.flags,
					req.con,
					req.loc,
					req.seq,
					0U
				};
				const auto fullBuffer = s.core->Query(s.duelPtr, qInfo);
				const auto query = DeserializeSingleQueryBuffer(fullBuffer);
				const auto ownerBuffer = SerializeSingleQuery(query, false);
				const auto strippedBuffer = SerializeSingleQuery(query, true);
				s.replay->RecordMsg(MakeUpdateCardMsg(req.con, req.loc, req.seq, fullBuffer));
				auto strippedMsg = MakeMsg(strippedBuffer);
				uint8_t team = GetSwappedTeam(req.con);
				SendToTeam(team, MakeMsg(ownerBuffer));
				SendToTeam(1U - team, strippedMsg);
				SendToSpectators(SaveToSpectatorCache(s, std::move(strippedMsg)));
			}
			else /*if(std::holds_alternative<QueryLocationRequest>(reqVar))*/
			{
				const auto& req = std::get<QueryLocationRequest>(reqVar);
				auto MakeMsg = [&](const QueryBuffer& qb) -> YGOPro::STOCMsg
				{
					return MakeGameMsg(MakeUpdateDataMsg(req.con, req.loc, qb));
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
				const auto fullBuffer = s.core->QueryLocation(s.duelPtr, qInfo);
				s.replay->RecordMsg(MakeUpdateDataMsg(req.con, req.loc, fullBuffer));
				if(req.loc == LOCATION_DECK)
					continue;
				if(req.loc == LOCATION_EXTRA)
				{
					SendToTeam(team, MakeMsg(fullBuffer));
					continue;
				}
				const auto query = DeserializeLocationQueryBuffer(fullBuffer);
				const auto ownerBuffer = SerializeLocationQuery(query, false);
				const auto strippedBuffer = SerializeLocationQuery(query, true);
				auto strippedMsg = MakeMsg(strippedBuffer);
				SendToTeam(team, MakeMsg(ownerBuffer));
				SendToTeam(1U - team, strippedMsg);
				SendToSpectators(SaveToSpectatorCache(s, std::move(strippedMsg)));
			}
		}
	};
	auto DistributeMsg = [&](const Msg& msg)
	{
		s.replay->RecordMsg(msg);
		switch(GetMessageDistributionType(msg))
		{
		case MsgDistType::MSG_DIST_TYPE_SPECIFIC_TEAM_DUELIST_STRIPPED:
		{
			uint8_t team = GetMessageReceivingTeam(msg);
			const auto sMsg = StripMessageForTeam(team, msg);
			auto& client = GetCurrentTeamClient(s, GetSwappedTeam(team));
			client.Send(MakeGameMsg(sMsg));
			break;
		}
		case MsgDistType::MSG_DIST_TYPE_SPECIFIC_TEAM_DUELIST:
		{
			uint8_t team = GetMessageReceivingTeam(msg);
			auto& client = GetCurrentTeamClient(s, GetSwappedTeam(team));
			client.Send(MakeGameMsg(msg));
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
				SaveToSpectatorCache(s, MakeGameMsg(msg)));
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
			SendToSpectators(
				SaveToSpectatorCache(s,
					MakeGameMsg(StripMessageForTeam(1U, sMsgs[0U]))));
			break;
		}
		case MsgDistType::MSG_DIST_TYPE_EVERYONE:
		{
			SendToAll(SaveToSpectatorCache(s, MakeGameMsg(msg)));
			break;
		}
		}
	};
	auto PostAnalyzeMsg = [&](const Msg& msg) -> std::optional<DuelFinishReason>
	{
		using Reason = DuelFinishReason::Reason;
		uint8_t msgType = GetMessageType(msg);
		if(msgType == MSG_RETRY)
		{
			LOG_ERROR(I18N::ROOM_DUELING_MSG_RETRY_RECEIVED);
			uint8_t winner = 1U - s.replier->Position().first;
			return DuelFinishReason{Reason::REASON_WRONG_RESPONSE, winner};
		}
		if(msgType == MSG_WIN)
		{
			uint8_t winner = (msg[1U] > 1U) ? 2U : GetSwappedTeam(msg[1U]);
			return DuelFinishReason{Reason::REASON_DUEL_WON, winner};
		}
		if(DoesMessageRequireAnswer(msgType))
		{
			SendToAllExcept(*s.replier, MakeGameMsg({MSG_WAITING}));
			if(hostInfo.timeLimitInSeconds != 0U)
			{
				using namespace std::chrono;
				uint8_t team = GetSwappedTeam(GetMessageReceivingTeam(msg));
				// Time remaining in seconds.
				const auto tr = duration_cast<seconds>(s.timeRemaining[team]);
				// Apparent time remaining (time that is sent to clients).
				const auto atr = tr - GRACE_PERIOD;
				// Tick value (seconds as integer) clamped to 0.
				const auto ticks = uint16_t(std::max(atr.count(), {}));
				tagg.ExpiresAfter(team, tr);
				SendToAll(MakeTimeLimit(GetSwappedTeam(team), ticks));
			}
		}
		return std::nullopt;
	};
	auto ProcessSingleMsg = [&](const Msg& msg) -> std::optional<DuelFinishReason>
	{
		if(!PreAnalyzeMsg(msg))
			return std::nullopt;
		ProcessQueryRequests(GetPreDistQueryRequests(msg));
		DistributeMsg(msg);
		ProcessQueryRequests(GetPostDistQueryRequests(msg));
		return PostAnalyzeMsg(msg);
	};
	try
	{
		for(;;)
		{
			Core::IWrapper::DuelStatus status = s.core->Process(s.duelPtr);
			for(const auto& msg : SplitToMsgs(s.core->GetMessages(s.duelPtr)))
				if(auto dfrOpt = ProcessSingleMsg(msg); dfrOpt)
					return dfrOpt;
			if(status != Core::IWrapper::DuelStatus::DUEL_STATUS_CONTINUE)
				break;
		}
	}
	catch(Core::Exception& e)
	{
		LOG_ERROR(I18N::ROOM_DUELING_CORE_EXCEPT_PROCESSING, e.what());
		return CORE_EXC_REASON;
	}
	return std::nullopt;
}

StateVariant Context::Finish(State::Dueling& s, const DuelFinishReason& dfr) noexcept
{
	using Reason = DuelFinishReason::Reason;
	if(dfr.reason != Reason::REASON_CORE_CRASHED)
	{
		// NOTE: if the core crashes here, it'll be handled on next duel
		// creation. Or if the room lifetime ended, it wouldn't matter anyway
		// as the crash frees all the memory for us.
		try
		{
			s.core->DestroyDuel(s.duelPtr);
		}
		catch(Core::Exception& e)
		{
			LOG_INFO(I18N::ROOM_DUELING_CORE_EXCEPT_DESTRUCTOR, e.what());
		}
	}
	tagg.Cancel(0U);
	tagg.Cancel(1U);
	auto SendWinMsg = [&](uint8_t reason)
	{
		const std::vector<uint8_t> winMsg =
		{
			MSG_WIN,
			(dfr.winner != 2U) ? GetSwappedTeam(dfr.winner) : uint8_t(2U),
			reason
		};
		s.replay->RecordMsg(winMsg);
		SendToAll(MakeGameMsg(winMsg));
	};
	auto SendReplay = [&]()
	{
		s.replay->Serialize();
		svc.replayManager.Save(s.replayId, *s.replay);
		if(s.replay->Bytes().size() > YGOPro::STOCMsg::MAX_PAYLOAD_SIZE)
			SendToAll(MakeChat(CHAT_MSG_TYPE_ERROR, I18N::CLIENT_ROOM_REPLAY_TOO_BIG));
		else
			SendToAll(MakeSendReplay(s.replay->Bytes()));
		SendToAll(MakeOpenReplayPrompt());
	};
	auto* turnDecider = [&]() -> Client*
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
		SendReplay();
		if(hostInfo.bestOf <= 1) // Single.
			return State::Rematching{turnDecider, {}};
		// Match.
		duelsHad++;
		if(dfr.winner != 2U)
		{
			wins[dfr.winner] += (s.matchKillReason.has_value()) ? neededWins : 1U;
			if(wins[dfr.winner] >= neededWins)
			{
				SendToAll(MakeDuelEnd());
				return State::Closing{};
			}
		}
		else if(!IsTiebreaking() && duelsHad >= hostInfo.bestOf)
		{
			SendToAll(MakeDuelEnd());
			return State::Closing{};
		}
		return State::Sidedecking{turnDecider, {}};
	}
	case Reason::REASON_CORE_CRASHED:
	{
		SendToAll(MakeChat(CHAT_MSG_TYPE_ERROR, I18N::CLIENT_ROOM_CORE_EXCEPT));
		SendWinMsg(WIN_REASON_INTERNAL_ERROR);
		SendReplay();
		if(hostInfo.bestOf <= 1)
			return State::Rematching{turnDecider, {}};
		return State::Sidedecking{turnDecider, {}};
	}
	case Reason::REASON_CONNECTION_LOST:
	{
		SendWinMsg(WIN_REASON_CONNECTION_LOST);
		[[fallthrough]];
	}
	default:
	{
		SendReplay();
		SendToAll(MakeDuelEnd());
		return State::Closing{};
	}
	}
}

const YGOPro::STOCMsg& Context::SaveToSpectatorCache(
	State::Dueling& s,
	YGOPro::STOCMsg&& msg) noexcept
{
	s.spectatorCache.emplace_back(msg);
	return s.spectatorCache.back();
}

} // namespace Ignis::Multirole::Room
