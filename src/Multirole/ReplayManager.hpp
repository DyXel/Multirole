#ifndef REPLAYMANAGER_HPP
#define REPLAYMANAGER_HPP
#include <atomic>
#include <string>
#include <string_view>

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
	std::atomic<uint64_t> currentId;
};

} // namespace Ignis::Multirole

#endif // REPLAYMANAGER_HPP
