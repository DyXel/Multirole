#include "CoreProvider.hpp"

#include <spdlog/spdlog.h>

#include "Core/DLWrapper.hpp"

namespace Ignis::Multirole
{

CoreProvider::CoreProvider(std::string_view fnRegexStr)
	:
	fnRegex(fnRegexStr.data()),
	type(CoreType::SHARED),
	loadPerCall(false)
{}

void CoreProvider::SetLoadProperties(CoreType typeValue, bool loadPerCallValue)
{
	type = typeValue;
	loadPerCall = loadPerCallValue;
}

CoreProvider::CorePtr CoreProvider::GetCore()
{
	std::lock_guard<std::mutex> lock(mCore);
	if(loadPerCall)
		return LoadCore();
	return core;
}

void CoreProvider::OnAdd(std::string_view path, const PathVector& fileList)
{
	OnGitUpdate(path, fileList);
}

void CoreProvider::OnDiff(std::string_view path, const GitDiff& diff)
{
	OnGitUpdate(path, diff.added);
}

// private

CoreProvider::CorePtr CoreProvider::LoadCore() const
{
	if(type == CoreType::SHARED)
		return std::make_shared<Core::DLWrapper>(corePath);
	throw std::runtime_error("No other core type is implemented.");
}

void CoreProvider::OnGitUpdate(std::string_view path, const PathVector& fl)
{
	std::lock_guard<std::mutex> lock(mCore);
	for(auto it = fl.rbegin(); it != fl.rend(); ++it)
	{
		if(!std::regex_match(*it, fnRegex))
			continue;
		corePath = path;
		corePath += *it;
		break;
	}
	if(!loadPerCall)
		core = LoadCore();
}

} // namespace Ignis::Multirole
