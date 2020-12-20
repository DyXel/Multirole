#ifndef REPLAYMANAGER_HPP
#define REPLAYMANAGER_HPP
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

class ReplayManager
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

#endif // REPLAYMANAGER_HPP
