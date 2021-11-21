#include "CoreProvider.hpp"

#include <fstream>

#include <boost/filesystem.hpp>

#include "LogHandler.hpp"
#define LOG_INFO(...) lh.Log(ServiceType::CORE_PROVIDER, Level::INFO, __VA_ARGS__)
#define LOG_ERROR(...) lh.Log(ServiceType::CORE_PROVIDER, Level::ERROR, __VA_ARGS__)
#include "../I18N.hpp"
#include "../Core/DLWrapper.hpp"
#include "../Core/HornetWrapper.hpp"

namespace Ignis::Multirole
{

Service::CoreProvider::CoreProvider(Service::LogHandler& lh, std::string_view fnRegexStr, const boost::filesystem::path& tmpDir, CoreType type, bool loadPerCall)
	:
	lh(lh),
	fnRegex(fnRegexStr.data()),
	tmpDir(tmpDir),
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

Service::CoreProvider::~CoreProvider() noexcept
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

void Service::CoreProvider::OnAdd(const boost::filesystem::path& path, const PathVector& fileList)
{
	OnGitUpdate(path, fileList);
}

void Service::CoreProvider::OnDiff(const boost::filesystem::path& path, const GitDiff& diff)
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

void Service::CoreProvider::OnGitUpdate(const boost::filesystem::path& path, const PathVector& fileList)
{
	auto it = fileList.begin();
	for(; it != fileList.end(); ++it)
		if(std::regex_match(it->string(), fnRegex))
			break;
	if(it == fileList.end())
	{
		if(shouldTest)
			throw std::runtime_error(I18N::CORE_PROVIDER_CORE_NOT_FOUND_IN_REPO);
		return;
	}
	std::scoped_lock lock(mCore);
	const boost::filesystem::path oldCoreLoc = coreLoc;
	const boost::filesystem::path repoCore = (path / *it).lexically_normal();
	coreLoc = (tmpDir / fmt::format("{}-{}-{}", uniqueId, loadCount++, repoCore.filename().string())).lexically_normal();
	LOG_INFO(I18N::CORE_PROVIDER_COPYING_CORE_FILE, repoCore.string(), coreLoc.string());
	pLocs.emplace_back(coreLoc);
	boost::filesystem::copy_file(repoCore, coreLoc);
	if(!boost::filesystem::exists(coreLoc))
	{
		LOG_ERROR(I18N::CORE_PROVIDER_FAILED_TO_COPY_CORE_FILE);
		coreLoc = oldCoreLoc;
	}
	try
	{
		auto core = LoadCore();
		const auto ver = core->Version();
		LOG_INFO(I18N::CORE_PROVIDER_VERSION_REPORTED, ver.first, ver.second);
	}
	catch(Core::Exception& e)
	{
		if(shouldTest)
			throw;
		LOG_ERROR(I18N::CORE_PROVIDER_ERROR_WHILE_TESTING, coreLoc.string(), e.what());
		coreLoc = oldCoreLoc;
		return;
	}
	shouldTest = false;
	if(!loadPerCall)
		core = LoadCore();
}

} // namespace Ignis::Multirole
