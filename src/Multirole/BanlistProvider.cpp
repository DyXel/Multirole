#include "BanlistProvider.hpp"

#include <fstream>

#include <spdlog/spdlog.h>

namespace Ignis::Multirole
{

BanlistProvider::BanlistProvider(std::string_view fnRegexStr) :
	fnRegex(fnRegexStr.data())
{}

const YGOPro::Banlist& BanlistProvider::GetBanlistByHash(YGOPro::BanlistHash hash)
{
	std::lock_guard<std::mutex> lock (mBanlists);
	return banlists.at(hash);
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
			std::fstream f(fullPath, std::ios_base::in);
			YGOPro::ParseForBanlists(f, tmp);
		}
		catch(const std::exception& e)
		{
			spdlog::error("BanlistProvider: Couldn't load banlist: {:s}", e.what());
		}
	}
	std::lock_guard<std::mutex> lock (mBanlists);
	// Delete banlists that have the same hash (`merge` does not replace them)
	for(const auto& kv : tmp)
		banlists.erase(kv.first);
	banlists.merge(tmp);
}

} // namespace Ignis::Multirole
