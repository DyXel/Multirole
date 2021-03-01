#include "ScriptProvider.hpp"

#include <stdexcept> // std::runtime_error
#include <fstream>
#include <sstream>

#include <spdlog/spdlog.h>

namespace Ignis::Multirole
{

// public

Service::ScriptProvider::ScriptProvider(std::string_view fnRegexStr) :
	fnRegex(fnRegexStr.data())
{}

void Service::ScriptProvider::OnAdd(std::string_view path, const PathVector& fileList)
{
	LoadScripts(path, fileList);
}

void Service::ScriptProvider::OnDiff(std::string_view path, const GitDiff& diff)
{
	LoadScripts(path, diff.added);
}

std::string Service::ScriptProvider::ScriptFromFilePath(std::string_view fp) const
{
	std::shared_lock lock(mScripts);
	if(auto search = scripts.find(fp.data()); search != scripts.end())
		return search->second;
	return std::string();
}

// private

void Service::ScriptProvider::LoadScripts(std::string_view path, const PathVector& fileList)
{
	int total = 0;
	spdlog::info("ScriptProvider: Loading {:d} files...", fileList.size());
	std::string fullPath(path);
	std::scoped_lock lock(mScripts);
	for(const auto& fn : fileList)
	{
		if(!std::regex_match(fn, fnRegex))
			continue;
		fullPath.resize(path.size());
		fullPath += fn;
		// Open file, checking if it exists
		std::ifstream file(fullPath, std::ifstream::binary);
		if(!file.is_open())
		{
			spdlog::error("ScriptProvider: Couldnt open file {:s}", fullPath);
			continue;
		}
		// Lambda to remove all subdirectories of a given filename
		auto FilenameFromPath = [](std::string_view str) -> std::string
		{
			static const auto npos = std::string::npos;
			std::size_t pos = str.rfind('/');
			if(pos != npos || (pos = str.rfind('\\')) != npos)
				return std::string(str.substr(pos + 1U));
			return std::string(str);
		};
		// Read actual file into memory and place into script map
		std::stringstream buffer;
		buffer << file.rdbuf();
		scripts.insert_or_assign(FilenameFromPath(fn), buffer.str());
		total++;
	}
	spdlog::info("ScriptProvider: Loaded {:d} files", total);
}

} // namespace Ignis::Multirole
