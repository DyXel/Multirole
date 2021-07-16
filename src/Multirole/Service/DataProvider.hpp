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
	DataProvider(Service::LogHandler& lh, std::string_view fnRegexStr);

	std::shared_ptr<YGOPro::CardDatabase> GetDatabase() const noexcept;

	// IGitRepoObserver overrides
	void OnAdd(const boost::filesystem::path& path, const PathVector& fileList) override;
	void OnDiff(const boost::filesystem::path& path, const GitDiff& diff) override;
private:
	Service::LogHandler& lh;
	const std::regex fnRegex;
	std::set<boost::filesystem::path> paths;
	std::shared_ptr<YGOPro::CardDatabase> db;
	mutable std::shared_mutex mDb;

	void ReloadDatabases() noexcept;
};

} // namespace Ignis::Multirole

#endif // SERVICE_DATAPROVIDER_HPP
