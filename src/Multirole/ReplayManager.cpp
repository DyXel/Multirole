#include "ReplayManager.hpp"

#include <fstream>
#include <spdlog/spdlog.h>

#include "YGOPro/Replay.hpp"

// Creates a folder on the given path.
// Returns true if succesful or if directory already existed, false otherwise.
bool MakeDir(std::string_view path);

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

bool MakeDir(std::string_view path)
{
	return CreateDirectoryA(path.data(), NULL) || ERROR_ALREADY_EXISTS == GetLastError();
}

#else
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

bool MakeDir(std::string_view path)
{
	return !mkdir(path.data(), 0777) || errno == EEXIST;
}

#endif

namespace Ignis::Multirole
{

constexpr auto BINARY_IN = std::ios_base::binary | std::ios_base::in;
constexpr auto BINARY_OUT = std::ios_base::binary | std::ios_base::out;

ReplayManager::ReplayManager(std::string_view path) :
	folder(path),
	lastIdPath(folder + "/lastId")
{
	if(!MakeDir(folder))
		throw std::runtime_error("ReplayManager: Could not make replay folder");
	uint64_t v = 1U;
	if(std::fstream f(lastIdPath, BINARY_IN); f.is_open())
		f.read(reinterpret_cast<char*>(&v), sizeof(v));
	else if(f.open(lastIdPath, BINARY_OUT); f.is_open())
		f.write(reinterpret_cast<char*>(&v), sizeof(v));
	else
		throw std::runtime_error("ReplayManager: Could not create lastId file");
	spdlog::info("ReplayManager: Current ID is {}", v);
	currentId = v;
}

ReplayManager::~ReplayManager()
{
	uint64_t v = currentId;
	if(std::fstream f(lastIdPath, BINARY_OUT); f.is_open())
		f.write(reinterpret_cast<char*>(&v), sizeof(v));
	else
		spdlog::error("ReplayManager: Could not save ID to file. Was {}", v);
}

void ReplayManager::Save(uint64_t id, const YGOPro::Replay& replay) const
{
	std::string finalPath(folder + "/" + std::to_string(id) + ".yrpX");
	const auto& bytes = replay.Bytes();
	if(std::fstream f(finalPath, BINARY_OUT); f.is_open())
		f.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	else
		spdlog::error("ReplayManager: Unable to save replay {}", finalPath);
}

uint64_t ReplayManager::NewId()
{
	return currentId++;
}

} // namespace Ignis::Multirole
