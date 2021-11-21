#include "BanlistProvider.hpp"

#include <fstream>

#include "LogHandler.hpp"
#define LOG_INFO(...) lh.Log(ServiceType::BANLIST_PROVIDER, Level::INFO, __VA_ARGS__)
#define LOG_ERROR(...) lh.Log(ServiceType::BANLIST_PROVIDER, Level::ERROR, __VA_ARGS__)
#include "../I18N.hpp"
#define YGOPRO_BANLIST_PARSER_IMPLEMENTATION
#include "../YGOPro/BanlistParser.hpp"

namespace Ignis::Multirole
{

Service::BanlistProvider::BanlistProvider(Service::LogHandler& lh, std::string_view fnRegexStr) :
	lh(lh),
	fnRegex(fnRegexStr.data())
{}

YGOPro::BanlistPtr Service::BanlistProvider::GetBanlistByHash(YGOPro::BanlistHash hash) const noexcept
{
	std::shared_lock lock(mBanlists);
	if(auto search = banlists.find(hash); search != banlists.end())
		return search->second;
	return nullptr;
}

void Service::BanlistProvider::OnAdd(const boost::filesystem::path& path, const PathVector& fileList)
{
	LoadBanlists(path, fileList);
}

void Service::BanlistProvider::OnDiff(const boost::filesystem::path& path, const GitDiff& diff)
{
	LoadBanlists(path, diff.added);
}

// private

void Service::BanlistProvider::LoadBanlists(const boost::filesystem::path& path, const PathVector& fileList) noexcept
{
	YGOPro::BanlistMap tmp;
	for(const auto& fn : fileList)
	{
		if(!std::regex_match(fn.string(), fnRegex))
			continue;
		auto fullPath = (path / fn).lexically_normal();
		LOG_INFO(I18N::BANLIST_PROVIDER_LOADING_ONE, fullPath.string());
		try
		{
			std::ifstream f(fullPath.native());
			YGOPro::ParseForBanlists(f, tmp);
		}
		catch(const std::exception& e)
		{
			LOG_ERROR(I18N::BANLIST_PROVIDER_COULD_NOT_LOAD_ONE, e.what());
		}
	}
	std::scoped_lock lock(mBanlists);
	// Delete banlists that have the same hash (`merge` does not replace them)
	for(const auto& kv : tmp)
		banlists.erase(kv.first);
	banlists.merge(tmp);
}

} // namespace Ignis::Multirole
