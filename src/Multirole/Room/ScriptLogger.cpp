#include "ScriptLogger.hpp"

#include "../I18N.hpp"
#include "../Service/LogHandler.hpp"
#include "../YGOPro/MsgCommon.hpp"

namespace Ignis::Multirole::Room
{

ScriptLogger::ScriptLogger(Service::LogHandler& lh, const YGOPro::HostInfo& hostInfo) :
	lh(lh),
	ec([](const YGOPro::HostInfo& hi) constexpr -> ErrorCategory
	{
		// NOTE: This check is heuristic at best.
		const auto flags = YGOPro::HostInfo::OrDuelFlags(hi.duelFlagsHigh, hi.duelFlagsLow);
		auto Check = [f=flags](uint64_t c) constexpr -> bool
		{
			// NOLINTNEXTLINE: DUEL_TCG_SEGOC_NONPUBLIC
			return (f == c) || (f == (c | 0x100000000U));
		};
		if(hi.banlistHash == 0U || hi.dontCheckDeckContent != 0U || hi.extraRules != 0U)
			return ErrorCategory::UNOFFICIAL;
		if(Check(0x2E800U)) // NOLINT: DUEL_MODE_MR5
			return ErrorCategory::OFFICIAL;
		if(Check(0x628000U)) // NOLINT: DUEL_MODE_SPEED
			return ErrorCategory::SPEED;
		if(Check(0x7F28200U)) // NOLINT: DUEL_MODE_RUSH
			return ErrorCategory::RUSH;
		return ErrorCategory::UNOFFICIAL;
	}(hostInfo)),
	prevMsg(),
	currMsg(),
	replayId(0U),
	turnCounter(0U)
{}

void ScriptLogger::SetReplayID(uint64_t rid)
{
	replayId = rid;
}

void ScriptLogger::SetTurnCounter(uint32_t count)
{
	turnCounter = count;
}

void ScriptLogger::Log(LogType type, std::string_view str)
{
	if(type == LogType::LOG_TYPE_ERROR)
	{
		currMsg += str;
	}
	else if(type == LogType::LOG_TYPE_FROM_SCRIPT)
	{
		currMsg += "User debug message: ";
		currMsg += str;
	}
	else if(type == LogType::LOG_TYPE_FOR_DEBUG)
	{
		currMsg = str;
		currMsg += '\n';
		return; // Do not log just yet.
	}
	if(currMsg != prevMsg)
		lh.Log(ec, replayId, turnCounter, (prevMsg = currMsg));
	currMsg.clear();
}

} // namespace Ignis::Multirole::Room
