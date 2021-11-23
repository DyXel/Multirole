#include "ReplayManager.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "LogHandler.hpp"
#define LOG_INFO(...) lh.Log(ServiceType::REPLAY_MANAGER, Level::INFO, __VA_ARGS__)
#define LOG_WARN(...) lh.Log(ServiceType::REPLAY_MANAGER, Level::WARN, __VA_ARGS__)
#define LOG_ERROR(...) lh.Log(ServiceType::REPLAY_MANAGER, Level::ERROR, __VA_ARGS__)
#include "../I18N.hpp"
#include "../YGOPro/Replay.hpp"

namespace Ignis::Multirole
{

namespace
{

using FileScopedLock = boost::interprocess::scoped_lock<boost::interprocess::file_lock>;

constexpr auto IOS_BINARY = std::ios_base::binary;
constexpr auto IOS_BINARY_IN_ATE = IOS_BINARY | std::ios_base::in | std::ios_base::ate;
constexpr auto IOS_BINARY_OUT = IOS_BINARY | std::ios_base::out;

inline uint64_t ReadId(std::fstream& f) noexcept
{
	uint64_t id{};
	f.seekg(0, std::ios_base::beg);
	f.read(reinterpret_cast<char*>(&id), sizeof(id));
	return id;
}

inline void WriteId(std::fstream& f, uint64_t id) noexcept
{
	f.seekg(0, std::ios_base::beg);
	f.write(reinterpret_cast<char*>(&id), sizeof(id));
}

} // namespace

Service::ReplayManager::ReplayManager(Service::LogHandler& lh, bool save, const boost::filesystem::path& dir) :
	lh(lh),
	save(save),
	dir(dir),
	lastId(dir / "lastId"),
	lastIdLock(dir / "lastId.lock"),
	mLastId()
{
	if(!save)
	{
		LOG_INFO(I18N::REPLAY_MANAGER_NOT_SAVING_REPLAYS);
		return;
	}
	using namespace boost::filesystem;
	if(!exists(dir) && !create_directory(dir))
		throw std::runtime_error(I18N::REPLAY_MANAGER_COULD_NOT_CREATE_DIR);
	if(!is_directory(dir))
		throw std::runtime_error(I18N::REPLAY_MANAGER_PATH_IS_FILE_NOT_DIR);
	uint64_t id = 1U;
	if(!exists(lastId))
	{
		std::fstream f(lastId.native(), IOS_BINARY_OUT);
		if(!f.is_open())
			throw std::runtime_error(I18N::REPLAY_MANAGER_ERROR_WRITING_INITIAL_ID);
		WriteId(f, id);
	}
	if(!exists(lastIdLock))
	{
		std::fstream f(lastIdLock.native(), IOS_BINARY_OUT);
		if(!f.is_open())
			throw std::runtime_error(I18N::REPLAY_MANAGER_ERROR_CREATING_LOCK);
	}
	lLastId = boost::interprocess::file_lock(lastIdLock.native().data());
	FileScopedLock plock(lLastId);
	if(std::fstream f(lastId.native(), IOS_BINARY_IN_ATE); f.is_open())
	{
		const auto fsize = f.tellg();
		f.clear();
		if(fsize == sizeof(id))
		{
			id = ReadId(f);
			LOG_INFO(I18N::REPLAY_MANAGER_CURRENT_ID, id);
			return;
		}
		LOG_WARN(I18N::REPLAY_MANAGER_LASTID_SIZE_CORRUPTED, fsize, sizeof(id));
	}
}

void Service::ReplayManager::Save(uint64_t id, const YGOPro::Replay& replay) const noexcept
{
	if(!save)
		return;
	const auto fn = dir / (std::to_string(id) + ".yrpX");
	const auto& bytes = replay.Bytes();
	if(std::fstream f(fn.native(), IOS_BINARY_OUT); f.is_open())
		f.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	else
		LOG_ERROR(I18N::REPLAY_MANAGER_UNABLE_TO_SAVE, fn.string());
}

uint64_t Service::ReplayManager::NewId() noexcept
{
	if(!save)
		return 0U;
	uint64_t prevId = 0U;
	uint64_t id = 0U;
	std::scoped_lock tlock(mLastId);
	FileScopedLock plock(lLastId);
	if(std::fstream f(lastId.native(), IOS_BINARY_IN_ATE); f.is_open())
	{
		const auto fsize = f.tellg();
		f.clear();
		if(fsize != sizeof(id))
		{
			LOG_ERROR(I18N::REPLAY_MANAGER_LASTID_SIZE_CORRUPTED, fsize, sizeof(id));
			return 0U;
		}
		id = ReadId(f);
	}
	else
	{
		LOG_ERROR(I18N::REPLAY_MANAGER_CANNOT_OPEN_LASTID);
		return 0U;
	}
	prevId = id++;
	if(std::fstream f(lastId.native(), IOS_BINARY_OUT); f.is_open())
	{
		WriteId(f, id);
		return prevId;
	}
	LOG_ERROR(I18N::REPLAY_MANAGER_CANNOT_WRITE_ID);
	return 0U;
}

} // namespace Ignis::Multirole
