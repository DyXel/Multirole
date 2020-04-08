#include "CoreProvider.hpp"

#include <spdlog/spdlog.h>

#include "DataProvider.hpp"
#include "ScriptProvider.hpp"

#include "Core/DynamicLinkWrapper.hpp"

namespace Ignis::Multirole
{

CoreProvider::CoreProvider(
	std::string_view fnRegexStr,
	DataProvider& dataProvider,
	ScriptProvider& scriptProvider)
	:
	fnRegex(fnRegexStr.data()),
	dataProvider(dataProvider),
	scriptProvider(scriptProvider),
	type(CoreType::SHARED),
	loadPerCall(false)
{}

void CoreProvider::SetLoadProperties(CoreType type, bool loadPerCall)
{
	type = type;
	loadPerCall = loadPerCall;
}

CoreProvider::CorePkg CoreProvider::GetCorePkg()
{
	std::lock_guard<std::mutex> lock(mCore);
	auto LoadOrGetCore = [&]() -> CorePtr
	{
		if(loadPerCall)
			return LoadCore();
		else
			return core;
	};
	auto db = dataProvider.GetDB();
	auto core = LoadOrGetCore();
	core->SetDataSupplier(db.get());
	core->SetLogger(this);
	return {std::move(db), std::move(core)};
}

void CoreProvider::OnAdd(std::string_view path, const PathVector& fileList)
{
	OnGitUpdate(path, fileList);
}

void CoreProvider::OnDiff(std::string_view path, const GitDiff& diff)
{
	OnGitUpdate(path, diff.added);
}

void CoreProvider::Log(LogType type, std::string_view str)
{
	spdlog::error("From core [{:d}]: {:s}", type, str);
}

// private

CoreProvider::CorePtr CoreProvider::LoadCore() const
{
	auto ChooseCoreType = [&]() -> CorePtr
	{
		if(type == CoreType::SHARED)
			return std::make_shared<Core::DynamicLinkWrapper>(corePath);
		throw std::runtime_error("No other core type is implemented.");
	};
	auto core = ChooseCoreType();
	core->SetScriptSupplier(&scriptProvider);
	return core;
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
