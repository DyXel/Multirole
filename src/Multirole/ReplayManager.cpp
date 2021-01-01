#include "ReplayManager.hpp"

#include <fstream>
#include <spdlog/spdlog.h>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "../FileSystem.hpp"
#include "YGOPro/Replay.hpp"

namespace Ignis::Multirole
{

constexpr auto IOS_BINARY = std::ios_base::binary;
constexpr auto IOS_BINARY_IN = IOS_BINARY | std::ios_base::in;
constexpr auto IOS_BINARY_OUT = IOS_BINARY | std::ios_base::out;

inline std::string MakeDirAndString(std::string_view path)
{
	if(!FileSystem::MakeDir(path))
		throw std::runtime_error("ReplayManager: Could not make replay folder");
	return std::string(path);
}

ReplayManager::ReplayManager(std::string_view path) :
	folder(MakeDirAndString(path)),
	lastIdPath(folder + "/lastId"),
	mLastId(),
	lLastId("./config.json")
{
	uint64_t id = 1U;
	boost::interprocess::scoped_lock<boost::interprocess::file_lock> plock(lLastId);
	if(std::fstream f(lastIdPath, IOS_BINARY_IN); f.is_open())
	{
		f.ignore(std::numeric_limits<std::streamsize>::max());
		std::streamsize fsize = f.gcount();
		f.clear();
		if(fsize == sizeof(id))
		{
			f.seekg(0, std::ios_base::beg);
			f.read(reinterpret_cast<char*>(&id), sizeof(id));
			spdlog::info("ReplayManager: Current ID is {}", id);
			return;
		}
	}
	// Write initial value in case the file didn't exist.
	if(std::fstream f(lastIdPath, IOS_BINARY_OUT); f.is_open())
	{
		f.write(reinterpret_cast<char*>(&id), sizeof(id));
		spdlog::info("ReplayManager: Started replay ID at {}", id);
		return;
	}
	spdlog::error("ReplayManager: Unable to write starting replay ID to file");
}

ReplayManager::~ReplayManager()
{}

void ReplayManager::Save(uint64_t id, const YGOPro::Replay& replay) const
{
	std::string finalPath(folder + "/" + std::to_string(id) + ".yrpX");
	const auto& bytes = replay.Bytes();
	if(std::fstream f(finalPath, IOS_BINARY_OUT); f.is_open())
		f.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	else
		spdlog::error("ReplayManager: Unable to save replay {}", finalPath);
}

constexpr const char* CORRUPTED_LASTID_FILE =
"ReplayManager: lastId file byte size corrupted:"
" It's {}. Should be {}";

uint64_t ReplayManager::NewId()
{
	uint64_t prevId, id;
	std::scoped_lock tlock(mLastId);
	boost::interprocess::scoped_lock<boost::interprocess::file_lock> plock(lLastId);
	if(std::fstream f(lastIdPath, IOS_BINARY_IN); f.is_open())
	{
		f.ignore(std::numeric_limits<std::streamsize>::max());
		std::streamsize fsize = f.gcount();
		f.clear();
		if(fsize != sizeof(id))
		{
			spdlog::error(CORRUPTED_LASTID_FILE, fsize, sizeof(id));
			return 0U;
		}
		f.seekg(0, std::ios_base::beg);
		f.read(reinterpret_cast<char*>(&id), sizeof(id));
	}
	else
	{
		spdlog::error("ReplayManager: lastId cannot be opened for reading");
		return 0U;
	}
	prevId = id++;
	if(std::fstream f(lastIdPath, IOS_BINARY_OUT); f.is_open())
	{
		f.write(reinterpret_cast<char*>(&id), sizeof(id));
		return prevId;
	}
	spdlog::error("ReplayManager: Unable to write next replay ID to file");
	return 0U;
}

} // namespace Ignis::Multirole
