#ifndef SERVICE_REPLAYMANAGER_HPP
#define SERVICE_REPLAYMANAGER_HPP
#include "../Service.hpp"

#include <mutex>
#include <string>
#include <string_view>

#include <boost/interprocess/sync/file_lock.hpp>

namespace YGOPro
{

class Replay;

} // namespace YGOPro

namespace Ignis::Multirole
{

class Service::ReplayManager
{
public:
	ReplayManager(std::string_view path);
	~ReplayManager();

	void Save(uint64_t id, const YGOPro::Replay& replay) const;

	uint64_t NewId();
private:
	const std::string folder;
	const std::string lastIdPath;
	std::mutex mLastId; // guarantees thread-safety
	boost::interprocess::file_lock lLastId; // guarantees process-safety
};

} // namespace Ignis::Multirole

#endif // SERVICE_REPLAYMANAGER_HPP
