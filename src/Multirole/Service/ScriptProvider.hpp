#ifndef SERVICE_SCRIPTPROVIDER_HPP
#define SERVICE_SCRIPTPROVIDER_HPP
#include "../Service.hpp"

#include <regex>
#include <unordered_map>
#include <shared_mutex>

#include "../IGitRepoObserver.hpp"
#include "../Core/IScriptSupplier.hpp"

namespace Ignis::Multirole
{

class Service::ScriptProvider final : public IGitRepoObserver, public Core::IScriptSupplier
{
public:
	ScriptProvider(std::string_view fnRegexStr);

	// IGitRepoObserver overrides
	void OnAdd(std::string_view path, const PathVector& fileList) override;
	void OnDiff(std::string_view path, const GitDiff& diff) override;

	// Core::IScriptSupplier overrides
	std::string ScriptFromFilePath(std::string_view fp) const override;
private:
	const std::regex fnRegex;
	std::unordered_map<std::string, std::string> scripts;
	mutable std::shared_mutex mScripts;

	void LoadScripts(std::string_view path, const PathVector& fileList);
};

} // namespace Ignis::Multirole

#endif // SERVICE_SCRIPTPROVIDER_HPP
