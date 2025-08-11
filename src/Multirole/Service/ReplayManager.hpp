#ifndef SERVICE_REPLAYMANAGER_HPP
#define SERVICE_REPLAYMANAGER_HPP
#include "../Service.hpp"

#include <filesystem>
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
	ReplayManager(Service::LogHandler& lh, bool save, const std::filesystem::path& dir);

	void Save(uint64_t id, const YGOPro::Replay& replay) const noexcept;

	uint64_t NewId() noexcept;
private:
	Service::LogHandler& lh;
	const bool save;
	const std::filesystem::path dir;
	const std::filesystem::path lastId;
	const std::filesystem::path lastIdLock;
	std::mutex mLastId; // guarantees thread-safety
	boost::interprocess::file_lock lLastId; // guarantees process-safety
};

} // namespace Ignis::Multirole

#endif // SERVICE_REPLAYMANAGER_HPP
