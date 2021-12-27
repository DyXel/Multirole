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
	ScriptProvider(Service::LogHandler& lh, std::string_view fnRegexStr);

	// IGitRepoObserver overrides
	void OnAdd(const boost::filesystem::path& path, const PathVector& fileList) override;
	void OnDiff(const boost::filesystem::path& path, const GitDiff& diff) override;

	// Core::IScriptSupplier overrides
	ScriptType ScriptFromFilePath(std::string_view fp) const noexcept override;
private:
	Service::LogHandler& lh;
	const std::regex fnRegex;
	std::unordered_map<std::string, ScriptType> scripts;
	mutable std::shared_mutex mScripts;

	void LoadScripts(const boost::filesystem::path& path, const PathVector& fileList) noexcept;
};

} // namespace Ignis::Multirole

#endif // SERVICE_SCRIPTPROVIDER_HPP
