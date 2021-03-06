#include "CoreProvider.hpp"

#include <fstream>
#include <spdlog/spdlog.h>

#include "../../FileSystem.hpp"
#include "../Core/DLWrapper.hpp"
#include "../Core/HornetWrapper.hpp"

namespace Ignis::Multirole
{

inline std::string MakeDirAndString(std::string_view path)
{
	if(!FileSystem::MakeDir(path))
		throw std::runtime_error("CoreProvider: Could not make temporal folder");
	return std::string(path);
}

Service::CoreProvider::CoreProvider(std::string_view fnRegexStr, std::string_view tmpPath, CoreType type, bool loadPerCall)
	:
	fnRegex(fnRegexStr.data()),
	tmpPath(MakeDirAndString(tmpPath)),
	type(type),
	loadPerCall(loadPerCall),
	uniqueId(std::chrono::system_clock::now().time_since_epoch().count()),
	loadCount(0U),
	shouldTest(true)
{}

Service::CoreProvider::~CoreProvider()
{
	// TODO: Delete shared library files created.
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
		return std::make_shared<Core::DLWrapper>(corePath);
	if (type == CoreType::HORNET)
		return std::make_shared<Core::HornetWrapper>(corePath);
	throw std::runtime_error("CoreProvider: No other core type is implemented.");
}

void Service::CoreProvider::OnGitUpdate(std::string_view path, const PathVector& fl)
{
	std::scoped_lock lock(mCore);
	const std::string oldCorePath = corePath;
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
	const auto gitFnPath = [&]() -> std::string
	{
		std::string str(path);
		str += *it;
		return str;
	}();
	// Lambda to remove all subdirectories of a given filename
	auto FilenameFromPath = [](std::string_view str) -> std::string
	{
		static const auto npos = std::string::npos;
		std::size_t pos = str.rfind('/');
		if(pos != npos || (pos = str.rfind('\\')) != npos)
			return std::string(str.substr(pos + 1U));
		return std::string(str);
	};
	corePath = tmpPath;
	corePath += '/';
	corePath += std::to_string(uniqueId);
	corePath += '-';
	corePath += std::to_string(loadCount);
	corePath += '-';
	corePath += FilenameFromPath(gitFnPath);
	loadCount++;
	spdlog::info("CoreProvider: Copying core from '{}' to '{}'...", gitFnPath, corePath);
	{
		std::ifstream src(gitFnPath, std::ios::binary);
		std::ofstream dst(corePath, std::ios::binary);
		dst << src.rdbuf();
		if(!dst.good())
		{
			spdlog::error("CoreProvider: Unable to copy file, reverting to old core.");
			corePath = oldCorePath;
			return;
		}
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
		spdlog::error("CoreProvider: Error while testing core '{}': {}", corePath, e.what());
		spdlog::info("CoreProvider: Reverting to old core: '{}'", oldCorePath);
		corePath = oldCorePath;
		return;
	}
	shouldTest = false;
	if(!loadPerCall)
		core = LoadCore();
}

} // namespace Ignis::Multirole
