#include "DataProvider.hpp"

#include <cstring> // std::memset
#include <stdexcept> // std::runtime_error

#include <sqlite3.h>

#include "LogHandler.hpp"
#define LOG_INFO(...) lh.Log(ServiceType::DATA_PROVIDER, Level::INFO, __VA_ARGS__)
#define LOG_ERROR(...) lh.Log(ServiceType::DATA_PROVIDER, Level::ERROR, __VA_ARGS__)
#include "../I18N.hpp"
#include "../YGOPro/CardDatabase.hpp"

namespace Ignis::Multirole
{

// public

Service::DataProvider::DataProvider(Service::LogHandler& lh, std::string_view fnRegexStr) :
	lh(lh),
	fnRegex(fnRegexStr.data())
{}

std::shared_ptr<YGOPro::CardDatabase> Service::DataProvider::GetDatabase() const noexcept
{
	std::shared_lock lock(mDb);
	return db;
}

void Service::DataProvider::OnAdd(const boost::filesystem::path& path, const PathVector& fileList)
{
	// Filter and add to set of dbs
	for(const auto& fn : fileList)
	{
		if(!std::regex_match(fn.string(), fnRegex))
			continue;
		paths.insert((path / fn).lexically_normal());
	}
	ReloadDatabases();
}

void Service::DataProvider::OnDiff(const boost::filesystem::path& path, const GitDiff& diff)
{
	// Filter and remove from sets of dbs
	for(const auto& fn : diff.removed)
	{
		if(!std::regex_match(fn.string(), fnRegex))
			continue;
		paths.erase((path / fn).lexically_normal());
	}
	// Filter and add to set of dbs
	for(const auto& fn : diff.added)
	{
		if(!std::regex_match(fn.string(), fnRegex))
			continue;
		paths.insert((path / fn).lexically_normal());
	}
	ReloadDatabases();
}

// private

void Service::DataProvider::ReloadDatabases() noexcept
{
	auto newDb = std::make_shared<YGOPro::CardDatabase>();
	for(const auto& path : paths)
	{
		LOG_INFO(I18N::DATA_PROVIDER_LOADING_ONE, path.string());
		if(!newDb->Merge(path.string()))
			LOG_ERROR(I18N::DATA_PROVIDER_COULD_NOT_MERGE);
	}
	std::scoped_lock lock(mDb);
	db = newDb;
}

} // namespace Ignis::Multirole
