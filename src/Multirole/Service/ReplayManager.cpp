#include "ReplayManager.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <spdlog/spdlog.h>

#include "../YGOPro/Replay.hpp"

namespace Ignis::Multirole
{

constexpr auto IOS_BINARY = std::ios_base::binary;
constexpr auto IOS_BINARY_IN = IOS_BINARY | std::ios_base::in;
constexpr auto IOS_BINARY_OUT = IOS_BINARY | std::ios_base::out;

Service::ReplayManager::ReplayManager(std::string_view dirStr) :
	dir(dirStr.data()),
	lastId(dir / "lastId"),
	mLastId()
{
	using namespace boost::filesystem;
	if(!exists(dir) && !create_directory(dir))
		throw std::runtime_error("ReplayManager: Could not create replay directory");
	if(!is_directory(dir))
		throw std::runtime_error("ReplayManager: Replay directory path points to a file");
	uint64_t id = 1U;
	if(!exists(lastId))
	{
		std::fstream f(lastId, IOS_BINARY_OUT);
		if(!f.is_open())
			throw std::runtime_error("ReplayManager: Unable to write initial id!");
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
			spdlog::info("ReplayManager: Current ID is {}", id);
			return;
		}
		spdlog::warn("ReplayManager: lastId size is not {}", sizeof(id));
	}
}

void Service::ReplayManager::Save(uint64_t id, const YGOPro::Replay& replay) const
{
	const auto fn = dir / (std::to_string(id) + ".yrpX");
	const auto& bytes = replay.Bytes();
	if(std::fstream f(fn, IOS_BINARY_OUT); f.is_open())
		f.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	else
		spdlog::error("ReplayManager: Unable to save replay {}", fn.string());
}

constexpr const char* CORRUPTED_LASTID_FILE =
"ReplayManager: lastId file byte size corrupted:"
" It's {}. Should be {}";

uint64_t Service::ReplayManager::NewId()
{
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
	if(std::fstream f(lastId, IOS_BINARY_OUT); f.is_open())
	{
		f.write(reinterpret_cast<char*>(&id), sizeof(id));
		return prevId;
	}
	spdlog::error("ReplayManager: Unable to write next replay ID to file");
	return 0U;
}

} // namespace Ignis::Multirole
