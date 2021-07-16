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
	BanlistProvider(Service::LogHandler& lh, std::string_view fnRegexStr);

	YGOPro::BanlistPtr GetBanlistByHash(YGOPro::BanlistHash hash) const noexcept;

	// IGitRepoObserver overrides
	void OnAdd(const boost::filesystem::path& path, const PathVector& fileList) override;
	void OnDiff(const boost::filesystem::path& path, const GitDiff& diff) override;
private:
	Service::LogHandler& lh;
	const std::regex fnRegex;
	YGOPro::BanlistMap banlists;
	mutable std::shared_mutex mBanlists;

	void LoadBanlists(const boost::filesystem::path& path, const PathVector& fileList) noexcept;
};

} // namespace Ignis::Multirole

#endif // SERVICE_BANLISTPROVIDER_HPP
