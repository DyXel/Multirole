#include "ScriptProvider.hpp"

#include <fstream>
#include <mutex>
#include <sstream>
#include <stdexcept> // std::runtime_error

#include "LogHandler.hpp"
#define LOG_INFO(...) lh.Log(ServiceType::SCRIPT_PROVIDER, Level::INFO, __VA_ARGS__)
#define LOG_ERROR(...) lh.Log(ServiceType::SCRIPT_PROVIDER, Level::ERROR, __VA_ARGS__)
#include "../I18N.hpp"

namespace Ignis::Multirole
{

// public

Service::ScriptProvider::ScriptProvider(Service::LogHandler& lh, std::string_view fnRegexStr) :
	lh(lh),
	fnRegex(fnRegexStr.data())
{}

void Service::ScriptProvider::OnAdd(const boost::filesystem::path& path, const PathVector& fileList)
{
	LoadScripts(path, fileList);
}

void Service::ScriptProvider::OnDiff(const boost::filesystem::path& path, const GitDiff& diff)
{
	LoadScripts(path, diff.added);
}

Core::IScriptSupplier::ScriptType Service::ScriptProvider::ScriptFromFilePath(std::string_view fp) const noexcept
{
	std::shared_lock lock(mScripts);
	if(auto search = scripts.find(fp.data()); search != scripts.end())
		return search->second;
	return nullptr;
}

// private

void Service::ScriptProvider::LoadScripts(const boost::filesystem::path& path, const PathVector& fileList) noexcept
{
	int total = 0;
	LOG_INFO(I18N::SCRIPT_PROVIDER_LOADING_FILES, fileList.size());
	std::scoped_lock lock(mScripts);
	for(const auto& fn : fileList)
	{
		if(!std::regex_match(fn.string(), fnRegex))
			continue;
		const auto fullPath = (path / fn).lexically_normal();
		// Open file, checking if it exists
		std::ifstream file(fullPath.native(), std::ifstream::binary);
		if(!file.is_open())
		{
			LOG_ERROR(I18N::SCRIPT_PROVIDER_COULD_NOT_OPEN, fullPath.string());
			continue;
		}
		// Read actual file into memory and place into script map
		std::stringstream buffer;
		buffer << file.rdbuf();
		scripts.insert_or_assign(fn.filename().string(), std::make_shared<const std::string>(buffer.str()));
		total++;
	}
	LOG_INFO(I18N::SCRIPT_PROVIDER_TOTAL_FILES_LOADED, total);
}

} // namespace Ignis::Multirole
