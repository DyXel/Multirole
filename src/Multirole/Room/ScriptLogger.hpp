#ifndef ROOM_SCRIPTLOGGER_HPP
#define ROOM_SCRIPTLOGGER_HPP
#include <string>
#include "../Core/ILogger.hpp"
#include "../Service.hpp"
#include "../Service/LogHandler/Constants.hpp"

namespace YGOPro
{

struct HostInfo;

} // namespace YGOPro

namespace Ignis::Multirole::Room
{

class ScriptLogger final : public Core::ILogger
{
public:
	ScriptLogger(Service::LogHandler& lh, const YGOPro::HostInfo& hostInfo);

	void SetReplayID(uint64_t rid);
	void SetTurnCounter(uint32_t count);

	void Log(LogType type, std::string_view str) override;
private:
	Service::LogHandler& lh;
	const ErrorCategory ec;
	std::string prevMsg;
	std::string currMsg;
	uint64_t replayId;
	uint32_t turnCounter;
};

} // namespace Ignis::Multirole::Room

#endif // ROOM_SCRIPTLOGGER_HPP
