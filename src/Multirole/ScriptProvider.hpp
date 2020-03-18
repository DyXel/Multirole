#ifndef SCRIPTPROVIDER_HPP
#define SCRIPTPROVIDER_HPP
#include <regex>
#include <unordered_map>
#include <mutex>

#include "IGitRepoObserver.hpp"
#include "Core/IScriptSupplier.hpp"

namespace Ignis::Multirole
{

class ScriptProvider final : public IGitRepoObserver, public Core::IScriptSupplier
{
public:
	ScriptProvider(std::string_view fnRegexStr);

	// IGitRepoObserver overrides
	void OnAdd(std::string_view path, const PathVector& fileList) override;
	void OnDiff(std::string_view path, const GitDiff& diff) override;

	// Core::IScriptSupplier overrides
	std::string ScriptFromFilePath(std::string_view fp) override;
private:
	std::regex fnRegex;
	std::unordered_map<std::string, std::string> scripts;
	std::mutex mScripts;

	void LoadScripts(std::string_view path, const PathVector& fileList);
};

} // namespace Ignis::Multirole

#endif // SCRIPTPROVIDER_HPP
