#include "ScriptProvider.hpp"

#include <stdexcept> // std::runtime_error
#include <fstream>
#include <sstream>

#include <spdlog/spdlog.h>

namespace Ignis
{

namespace Multirole
{

// public

ScriptProvider::ScriptProvider(std::string_view fnRegexStr) :
	fnRegex(fnRegexStr.data())
{}

void ScriptProvider::OnAdd(std::string_view path, const PathVector& fullFileList)
{
	LoadScripts(path, fullFileList);
}

void ScriptProvider::OnReset(std::string_view path, const PathVector& deltaFileList)
{
	LoadScripts(path, deltaFileList);
}

std::string ScriptProvider::ScriptFromFilePath(std::string_view fp)
{
	std::lock_guard<std::mutex> lock(mScripts);
	if(auto search = scripts.find(fp.data()); search != scripts.end())
		return search->second;
	return std::string();
}

// private

void ScriptProvider::LoadScripts(std::string_view path, const PathVector& fileList)
{
	int total = 0;
	std::string fullPath(path);
	std::lock_guard<std::mutex> lock(mScripts);
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
			spdlog::error(FMT_STRING("ScriptProvider: Couldnt open file {:s}"), fullPath);
			continue;
		}
		// Lambda to remove all subdirectories of a given filename
		auto FilenameFromPath = [](std::string_view str) -> std::string
		{
			static const auto npos = std::string::npos;
			std::size_t pos = str.rfind('/');
			if(pos != npos || (pos = str.rfind('\\')) != npos)
				return std::string(str.substr(pos + 1u));
			return std::string(str);
		};
		// Read actual file into memory and place into script map
		std::stringstream buffer;
		buffer << file.rdbuf();
		scripts.emplace(FilenameFromPath(fn), buffer.str());
		total++;
	}
	spdlog::info(FMT_STRING("ScriptProvider: loaded {:d} files"), total);
}

} // namespace Multirole

} // namespace Ignis
