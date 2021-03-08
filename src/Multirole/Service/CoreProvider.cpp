#include "CoreProvider.hpp"

#include <fstream>

#include <boost/filesystem.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "../Core/DLWrapper.hpp"
#include "../Core/HornetWrapper.hpp"

namespace Ignis::Multirole
{

Service::CoreProvider::CoreProvider(std::string_view fnRegexStr, std::string_view tmpDirStr, CoreType type, bool loadPerCall)
	:
	fnRegex(fnRegexStr.data()),
	tmpDir(tmpDirStr.data()),
	type(type),
	loadPerCall(loadPerCall),
	uniqueId(std::chrono::system_clock::now().time_since_epoch().count()),
	loadCount(0U),
	shouldTest(true)
{
	using namespace boost::filesystem;
	if(!exists(tmpDir) && !create_directory(tmpDir))
		throw std::runtime_error("CoreProvider: Could not create temporal directory");
	if(!is_directory(tmpDir))
		throw std::runtime_error("CoreProvider: Temporal directory path points to a file");
}

Service::CoreProvider::~CoreProvider()
{
	for(const auto& fn : pLocs)
		boost::filesystem::remove(fn);
}

Service::CoreProvider::CorePtr Service::CoreProvider::GetCore() const
{
	std::shared_lock lock(mCore);
	if(loadPerCall)
		return LoadCore();
	return core;
}

void Service::CoreProvider::OnAdd(std::string_view path, const PathVector& fileList)
{
	OnGitUpdate(path, fileList);
}

void Service::CoreProvider::OnDiff(std::string_view path, const GitDiff& diff)
{
	OnGitUpdate(path, diff.added);
}

// private

Service::CoreProvider::CorePtr Service::CoreProvider::LoadCore() const
{
	if(type == CoreType::SHARED)
		return std::make_shared<Core::DLWrapper>(coreLoc.string());
	if (type == CoreType::HORNET)
		return std::make_shared<Core::HornetWrapper>(coreLoc.string());
	throw std::runtime_error("CoreProvider: No other core type is implemented");
}

void Service::CoreProvider::OnGitUpdate(std::string_view path, const PathVector& fl)
{
	auto it = fl.begin();
	for(; it != fl.end(); ++it)
		if(std::regex_match(*it, fnRegex))
			break;
	if(it == fl.end())
	{
		if(shouldTest)
			throw std::runtime_error("CoreProvider: Core not found in repository!");
		return;
	}
	std::scoped_lock lock(mCore);
	const boost::filesystem::path oldCoreLoc = coreLoc;
	const boost::filesystem::path repoCore = [&]()
	{
		boost::filesystem::path fullFn(path.data());
		fullFn /= *it;
		return fullFn;
	}();
	coreLoc = tmpDir / fmt::format("{}-{}-{}", uniqueId, loadCount++, repoCore.filename().string());
	spdlog::info("CoreProvider: Copying core from '{}' to '{}'...", repoCore.string(), coreLoc.string());
	pLocs.emplace_back(coreLoc);
	boost::filesystem::copy_file(repoCore, coreLoc);
	if(!boost::filesystem::exists(coreLoc))
	{
		spdlog::error("CoreProvider: Failed to copy core file! Re-testing old one");
		coreLoc = oldCoreLoc;
	}
	try
	{
		auto core = LoadCore();
		const auto ver = core->Version();
		spdlog::info("CoreProvider: Version reported by core: {}.{}", ver.first, ver.second);
	}
	catch(Core::Exception& e)
	{
		if(shouldTest)
			throw;
		spdlog::error("CoreProvider: Error while testing core '{}': {}", coreLoc.string(), e.what());
		spdlog::info("CoreProvider: Reverting to old core: '{}'", oldCoreLoc.string());
		coreLoc = oldCoreLoc;
		return;
	}
	shouldTest = false;
	if(!loadPerCall)
		core = LoadCore();
}

} // namespace Ignis::Multirole
