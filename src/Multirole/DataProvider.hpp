#ifndef DATAPROVIDER_HPP
#define DATAPROVIDER_HPP
#include <regex>
#include <memory>
#include <mutex>
#include <set>

#include "CardDatabase.hpp"
#include "IGitRepoObserver.hpp"

namespace Ignis::Multirole
{

class DataProvider final : public IGitRepoObserver
{
public:
	DataProvider(std::string_view fnRegexStr);

	std::shared_ptr<CardDatabase> GetDB();

	// IGitRepoObserver overrides
	void OnAdd(std::string_view path, const PathVector& fileList) override;
	void OnDiff(std::string_view path, const GitDiff& diff) override;
private:
	const std::regex fnRegex;
	std::set<std::string> paths;
	std::shared_ptr<CardDatabase> db;
	std::mutex mDb;

	void ReloadDBs();
};

} // namespace Ignis::Multirole

#endif // DATAPROVIDER_HPP
