#ifndef SERVICE_REPLAYMANAGER_HPP
#define SERVICE_REPLAYMANAGER_HPP
#include "../Service.hpp"

#include <mutex>
#include <string>
#include <string_view>

#include <boost/filesystem/path.hpp>
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
	ReplayManager(std::string_view dirStr);

	void Save(uint64_t id, const YGOPro::Replay& replay) const;

	uint64_t NewId();
private:
	const boost::filesystem::path dir;
	const boost::filesystem::path lastId;
	std::mutex mLastId; // guarantees thread-safety
	boost::interprocess::file_lock lLastId; // guarantees process-safety
};

} // namespace Ignis::Multirole

#endif // SERVICE_REPLAYMANAGER_HPP
