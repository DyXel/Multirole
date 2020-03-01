#ifndef SCRIPTPROVIDER_HPP
#define SCRIPTPROVIDER_HPP
#include <regex>
#include <unordered_map>
#include <mutex>

#include "IGitRepoObserver.hpp"
#include "Core/IScriptSupplier.hpp"

namespace Ignis
{

namespace Multirole
{

class IAsyncLogger;

class ScriptProvider final : public IGitRepoObserver, public Core::IScriptSupplier
{
public:
	ScriptProvider(IAsyncLogger& l, std::string_view fnRegexStr);

	// IGitRepoObserver overrides
	void OnAdd(std::string_view path, const PathVector& fullFileList) override;
	void OnReset(std::string_view path, const PathVector& deltaFileList) override;

	// Core::IScriptSupplier overrides
	std::string ScriptFromFilePath(std::string_view fp) override;
private:
	IAsyncLogger& logger;
	std::regex fnRegex;
	std::unordered_map<std::string, std::string> scripts;
	std::mutex mScripts;

	void LoadScripts(std::string_view path, const PathVector& fileList);
};

} // namespace Multirole

} // namespace Ignis

#endif // SCRIPTPROVIDER_HPP
