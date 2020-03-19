#include "DataProvider.hpp"

#include <cstring> // std::memset
#include <stdexcept> // std::runtime_error

#include <sqlite3.h>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace Ignis::Multirole
{

// public

DataProvider::DataProvider(std::string_view fnRegexStr) :
	fnRegex(fnRegexStr.data())
{}

std::shared_ptr<CardDatabase> DataProvider::GetDB()
{
	std::lock_guard<std::mutex> lock(mDb);
	return db;
}

void DataProvider::OnAdd(std::string_view path, const PathVector& fileList)
{
	std::string fullPath(path);
	// Filter and add to set of dbs
	for(const auto& fn : fileList)
	{
		if(!std::regex_match(fn, fnRegex))
			continue;
		fullPath.resize(path.size());
		fullPath += fn;
		paths.insert(fullPath);
	}
	ReloadDBs();
}

void DataProvider::OnDiff(std::string_view path, const GitDiff& diff)
{
	std::string fullPath(path);
	// Filter and remove from sets of dbs
	for(const auto& fn : diff.removed)
	{
		if(!std::regex_match(fn, fnRegex))
			continue;
		fullPath.resize(path.size());
		fullPath += fn;
		paths.insert(fullPath);
	}
	// Filter and add to set of dbs
	for(const auto& fn : diff.added)
	{
		if(!std::regex_match(fn, fnRegex))
			continue;
		fullPath.resize(path.size());
		fullPath += fn;
		paths.erase(fullPath);
	}
	ReloadDBs();
}

// private

void DataProvider::ReloadDBs()
{
	auto newDb = std::make_shared<CardDatabase>();
	for(const auto& path : paths)
	{
		spdlog::info("DataProvider: Loading up {:s}...", path);
		if(!newDb->Merge(path))
		{
			spdlog::error("DataProvider: Couldn't merge database");
		}
	}
	std::lock_guard<std::mutex> lock(mDb);
	db = newDb;
}

} // namespace Ignis::Multirole
