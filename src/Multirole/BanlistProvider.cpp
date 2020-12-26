#include "BanlistProvider.hpp"

#include <fstream>

#include <spdlog/spdlog.h>

#define YGOPRO_BANLIST_PARSER_IMPLEMENTATION
#include "YGOPro/BanlistParser.hpp"

namespace Ignis::Multirole
{

BanlistProvider::BanlistProvider(std::string_view fnRegexStr) :
	fnRegex(fnRegexStr.data())
{}

YGOPro::BanlistPtr BanlistProvider::GetBanlistByHash(YGOPro::BanlistHash hash) const
{
	std::shared_lock lock(mBanlists);
	if(auto search = banlists.find(hash); search != banlists.end())
		return search->second;
	return nullptr;
}

void BanlistProvider::OnAdd(std::string_view path, const PathVector& fileList)
{
	LoadBanlists(path, fileList);
}

void BanlistProvider::OnDiff(std::string_view path, const GitDiff& diff)
{
	LoadBanlists(path, diff.added);
}

// private

void BanlistProvider::LoadBanlists(std::string_view path, const PathVector& fileList)
{
	std::string fullPath(path);
	YGOPro::BanlistMap tmp;
	for(const auto& fn : fileList)
	{
		if(!std::regex_match(fn, fnRegex))
			continue;
		fullPath.resize(path.size());
		fullPath += fn;
		spdlog::info("BanlistProvider: Loading up {:s}...", fullPath);
		try
		{
			std::ifstream f(fullPath);
			YGOPro::ParseForBanlists(f, tmp);
		}
		catch(const std::exception& e)
		{
			spdlog::error("BanlistProvider: Couldn't load banlist: {:s}", e.what());
		}
	}
	std::scoped_lock lock(mBanlists);
	// Delete banlists that have the same hash (`merge` does not replace them)
	for(const auto& kv : tmp)
		banlists.erase(kv.first);
	banlists.merge(tmp);
}

} // namespace Ignis::Multirole
