#include "CoreProvider.hpp"

#include <fstream>

#include <boost/filesystem.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "../I18N.hpp"
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
		throw std::runtime_error(I18N::CORE_PROVIDER_COULD_NOT_CREATE_TMP_DIR);
	if(!is_directory(tmpDir))
		throw std::runtime_error(I18N::CORE_PROVIDER_PATH_IS_FILE_NOT_DIR);
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
	throw std::runtime_error(I18N::CORE_PROVIDER_WRONG_CORE_TYPE);
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
			throw std::runtime_error(I18N::CORE_PROVIDER_CORE_NOT_FOUND_IN_REPO);
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
	spdlog::info(I18N::CORE_PROVIDER_COPYING_CORE_FILE, repoCore.string(), coreLoc.string());
	pLocs.emplace_back(coreLoc);
	boost::filesystem::copy_file(repoCore, coreLoc);
	if(!boost::filesystem::exists(coreLoc))
	{
		spdlog::error(I18N::CORE_PROVIDER_FAILED_TO_COPY_CORE_FILE);
		coreLoc = oldCoreLoc;
	}
	try
	{
		auto core = LoadCore();
		const auto ver = core->Version();
		spdlog::info(I18N::CORE_PROVIDER_VERSION_REPORTED, ver.first, ver.second);
	}
	catch(Core::Exception& e)
	{
		if(shouldTest)
			throw;
		spdlog::error(I18N::CORE_PROVIDER_ERROR_WHILE_TESTING, coreLoc.string(), e.what());
		coreLoc = oldCoreLoc;
		return;
	}
	shouldTest = false;
	if(!loadPerCall)
		core = LoadCore();
}

} // namespace Ignis::Multirole
