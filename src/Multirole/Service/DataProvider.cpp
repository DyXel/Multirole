#include "DataProvider.hpp"

#include <cstring> // std::memset
#include <stdexcept> // std::runtime_error

#include <sqlite3.h>
#include <spdlog/spdlog.h>

#include "../I18N.hpp"
#include "../YGOPro/CardDatabase.hpp"

namespace Ignis::Multirole
{

// public

Service::DataProvider::DataProvider(std::string_view fnRegexStr) :
	fnRegex(fnRegexStr.data())
{}

std::shared_ptr<YGOPro::CardDatabase> Service::DataProvider::GetDatabase() const
{
	std::shared_lock lock(mDb);
	return db;
}

void Service::DataProvider::OnAdd(std::string_view path, const PathVector& fileList)
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
	ReloadDatabases();
}

void Service::DataProvider::OnDiff(std::string_view path, const GitDiff& diff)
{
	std::string fullPath(path);
	// Filter and remove from sets of dbs
	for(const auto& fn : diff.removed)
	{
		if(!std::regex_match(fn, fnRegex))
			continue;
		fullPath.resize(path.size());
		fullPath += fn;
		paths.erase(fullPath);
	}
	// Filter and add to set of dbs
	for(const auto& fn : diff.added)
	{
		if(!std::regex_match(fn, fnRegex))
			continue;
		fullPath.resize(path.size());
		fullPath += fn;
		paths.insert(fullPath);
	}
	ReloadDatabases();
}

// private

void Service::DataProvider::ReloadDatabases()
{
	auto newDb = std::make_shared<YGOPro::CardDatabase>();
	for(const auto& path : paths)
	{
		spdlog::info(I18N::DATA_PROVIDER_LOADING_ONE, path);
		if(!newDb->Merge(path))
			spdlog::error(I18N::DATA_PROVIDER_COULD_NOT_MERGE);
	}
	std::scoped_lock lock(mDb);
	db = newDb;
}

} // namespace Ignis::Multirole
