#ifndef SERVICE_DATAPROVIDER_HPP
#define SERVICE_DATAPROVIDER_HPP
#include "../Service.hpp"

#include <regex>
#include <memory>
#include <shared_mutex>
#include <set>

#include "../IGitRepoObserver.hpp"

namespace YGOPro
{

class CardDatabase;

} // namespace YGOPro

namespace Ignis::Multirole
{

class Service::DataProvider final : public IGitRepoObserver
{
public:
	DataProvider(std::string_view fnRegexStr);

	std::shared_ptr<YGOPro::CardDatabase> GetDatabase() const;

	// IGitRepoObserver overrides
	void OnAdd(std::string_view path, const PathVector& fileList) override;
	void OnDiff(std::string_view path, const GitDiff& diff) override;
private:
	const std::regex fnRegex;
	std::set<std::string> paths;
	std::shared_ptr<YGOPro::CardDatabase> db;
	mutable std::shared_mutex mDb;

	void ReloadDatabases();
};

} // namespace Ignis::Multirole

#endif // SERVICE_DATAPROVIDER_HPP
