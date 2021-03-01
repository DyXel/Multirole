#ifndef SERVICE_BANLISTPROVIDER_HPP
#define SERVICE_BANLISTPROVIDER_HPP
#include "../Service.hpp"

#include <regex>
#include <shared_mutex>

#include "../IGitRepoObserver.hpp"
#include "../YGOPro/BanlistParser.hpp"

namespace Ignis::Multirole
{

class Service::BanlistProvider final : public IGitRepoObserver
{
public:
	BanlistProvider(std::string_view fnRegexStr);

	YGOPro::BanlistPtr GetBanlistByHash(YGOPro::BanlistHash hash) const;

	// IGitRepoObserver overrides
	void OnAdd(std::string_view path, const PathVector& fileList) override;
	void OnDiff(std::string_view path, const GitDiff& diff) override;
private:
	const std::regex fnRegex;
	YGOPro::BanlistMap banlists;
	mutable std::shared_mutex mBanlists;

	void LoadBanlists(std::string_view path, const PathVector& fileList);
};

} // namespace Ignis::Multirole

#endif // SERVICE_BANLISTPROVIDER_HPP
