#include "ReplayManager.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <spdlog/spdlog.h>

#include "../I18N.hpp"
#include "../YGOPro/Replay.hpp"

namespace Ignis::Multirole
{

constexpr auto IOS_BINARY = std::ios_base::binary;
constexpr auto IOS_BINARY_IN = IOS_BINARY | std::ios_base::in;
constexpr auto IOS_BINARY_OUT = IOS_BINARY | std::ios_base::out;

Service::ReplayManager::ReplayManager(bool save, std::string_view dirStr) :
	save(save),
	dir(dirStr.data()),
	lastId(dir / "lastId"),
	mLastId()
{
	if(!save)
	{
		spdlog::info(I18N::REPLAY_MANAGER_NOT_SAVING_REPLAYS);
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
		std::fstream f(lastId, IOS_BINARY_OUT);
		if(!f.is_open())
			throw std::runtime_error(I18N::REPLAY_MANAGER_ERROR_WRITING_INITIAL_ID);
		f.write(reinterpret_cast<char*>(&id), sizeof(id));
	}
	lLastId = boost::interprocess::file_lock(lastId.string().data());
	boost::interprocess::scoped_lock<boost::interprocess::file_lock> plock(lLastId);
	if(std::fstream f(lastId, IOS_BINARY_IN); f.is_open())
	{
		f.ignore(std::numeric_limits<std::streamsize>::max());
		std::streamsize fsize = f.gcount();
		f.clear();
		if(fsize == sizeof(id))
		{
			f.seekg(0, std::ios_base::beg);
			f.read(reinterpret_cast<char*>(&id), sizeof(id));
			spdlog::info(I18N::REPLAY_MANAGER_CURRENT_ID, id);
			return;
		}
		spdlog::warn(I18N::REPLAY_MANAGER_LASTID_SIZE_CORRUPTED, fsize, sizeof(id));
	}
}

void Service::ReplayManager::Save(uint64_t id, const YGOPro::Replay& replay) const
{
	if(!save)
		return;
	const auto fn = dir / (std::to_string(id) + ".yrpX");
	const auto& bytes = replay.Bytes();
	if(std::fstream f(fn, IOS_BINARY_OUT); f.is_open())
		f.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	else
		spdlog::error(I18N::REPLAY_MANAGER_UNABLE_TO_SAVE, fn.string());
}

uint64_t Service::ReplayManager::NewId()
{
	if(!save)
		return 0U;
	uint64_t prevId = 0U;
	uint64_t id = 0U;
	std::scoped_lock tlock(mLastId);
	boost::interprocess::scoped_lock<boost::interprocess::file_lock> plock(lLastId);
	if(std::fstream f(lastId, IOS_BINARY_IN); f.is_open())
	{
		f.ignore(std::numeric_limits<std::streamsize>::max());
		std::streamsize fsize = f.gcount();
		f.clear();
		if(fsize != sizeof(id))
		{
			spdlog::error(I18N::REPLAY_MANAGER_LASTID_SIZE_CORRUPTED, fsize, sizeof(id));
			return 0U;
		}
		f.seekg(0, std::ios_base::beg);
		f.read(reinterpret_cast<char*>(&id), sizeof(id));
	}
	else
	{
		spdlog::error(I18N::REPLAY_MANAGER_CANNOT_OPEN_LASTID);
		return 0U;
	}
	prevId = id++;
	if(std::fstream f(lastId, IOS_BINARY_OUT); f.is_open())
	{
		f.write(reinterpret_cast<char*>(&id), sizeof(id));
		return prevId;
	}
	spdlog::error(I18N::REPLAY_MANAGER_CANNOT_WRITE_ID);
	return 0U;
}

} // namespace Ignis::Multirole
