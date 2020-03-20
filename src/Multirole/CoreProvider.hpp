#ifndef COREPROVIDER_HPP
#define COREPROVIDER_HPP
#include <regex>
#include <memory>
#include <mutex>

#include "IGitRepoObserver.hpp"
#include "Core/IHighLevelWrapper.hpp"

namespace Ignis::Multirole
{

class DataProvider;
class ScriptProvider;

class CoreProvider : public IGitRepoObserver
{
public:
	enum CoreType
	{
		SHARED,
		HORNET,
	};
	using CorePtr = std::shared_ptr<Core::IHighLevelWrapper>;

	// The shared pointer to Core::IDataSupplier is needed so the object
	// can outlive the Core::IHighLevelWrapper that uses it.
	using CorePkg = std::pair<std::shared_ptr<Core::IDataSupplier>, CorePtr>;

	CoreProvider(std::string_view fnRegexStr, DataProvider& dataProvider,
	             ScriptProvider& scriptProvider);
	void SetLoadProperties(CoreType type, bool loadPerCall);

	// Will return a core instance based on the options set by SetLoadProperties
	// along with the Core::IDataSupplier used for that same core instance
	// already set as if by calling Core::IHighLevelWrapper::SetDataSupplier and
	// with the Data::ScriptProvider set as if by calling
	// Core::IHighLevelWrapper::SetScriptSupplier with `&scriptProvider`.
	CorePkg GetCorePkg();

	// IGitRepoObserver overrides
	void OnAdd(std::string_view path, const PathVector& fileList) override;
	void OnDiff(std::string_view path, const GitDiff& diff) override;
private:
	const std::regex fnRegex;
	DataProvider& dataProvider;
	ScriptProvider& scriptProvider;
	CoreType type;
	bool loadPerCall;
	std::string corePath;
	CorePtr core;
	std::mutex mCore; // used for both corePath and core.

	CorePtr LoadCore() const;

	void OnGitUpdate(std::string_view path, const PathVector& fl);
};

} // namespace Ignis::Multirole

#endif // COREPROVIDER_HPP