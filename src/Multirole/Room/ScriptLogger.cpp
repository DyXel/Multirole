#include "ScriptLogger.hpp"

#include "../Service/LogHandler.hpp"
#include "../YGOPro/MsgCommon.hpp"

namespace Ignis::Multirole::Room
{

ScriptLogger::ScriptLogger(Service::LogHandler& lh, const YGOPro::HostInfo& hostInfo) :
	lh(lh),
	ec([](const YGOPro::HostInfo& hi) constexpr -> decltype(ec)
	{
		// NOTE: This check is heuristic at best.
		const auto flags = YGOPro::HostInfo::OrDuelFlags(hi.duelFlagsHigh, hi.duelFlagsLow);
		if(hi.banlistHash == 0U || hi.dontCheckDeck != 0U || hi.extraRules != 0U)
			return ErrorCategory::UNOFFICIAL;
		else if(flags == 0x2E800U) // NOLINT: DUEL_MODE_MR5
			return ErrorCategory::OFFICIAL;
		else if(flags == 0x10002E800U) // NOLINT: DUEL_MODE_MR5 + TCG SEGOC
			return ErrorCategory::OFFICIAL;
		else if(flags == 0x628000U) // NOLINT: DUEL_MODE_SPEED
			return ErrorCategory::SPEED;
		else if(flags == 0x100628000U) // NOLINT: DUEL_MODE_SPEED + TCG SEGOC
			return ErrorCategory::SPEED;
		else if(flags == 0x7F28200U) // NOLINT: DUEL_MODE_RUSH
			return ErrorCategory::RUSH;
		else
			return ErrorCategory::UNOFFICIAL;
	}(hostInfo))
{}

void ScriptLogger::SetReplayID(uint64_t rid)
{
	replayId = rid;
}

void ScriptLogger::Log(LogType type, std::string_view str)
{
	lh.Log(ec, replayId, str);
}

} // namespace Ignis::Multirole::Room
