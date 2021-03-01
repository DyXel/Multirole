#ifndef MULTIROLE_SERVICE_HPP
#define MULTIROLE_SERVICE_HPP

namespace Ignis::Multirole
{

struct Service final
{
#define SERVICE(klass, member) class klass; klass& member;
	SERVICE(BanlistProvider, banlistProvider)
	SERVICE(CoreProvider, coreProvider)
	SERVICE(DataProvider, dataProvider)
// 	SERVICE(LogHandler, logHandler)
	SERVICE(ReplayManager, replayManager)
	SERVICE(ScriptProvider, scriptProvider)
#undef SERVICE
};

} // namespace Ignis::Multirole

#endif // MULTIROLE_SERVICE_HPP
