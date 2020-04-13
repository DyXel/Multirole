#ifndef COREPROVIDER_HPP
#define COREPROVIDER_HPP
#include <regex>
#include <memory>
#include <mutex>

#include "IGitRepoObserver.hpp"
#include "Core/IHighLevelWrapper.hpp"
#include "Core/ILogger.hpp"

namespace Ignis::Multirole
{

class CardDatabase;
class DataProvider;
class ScriptProvider;

class CoreProvider : public IGitRepoObserver, public Core::ILogger
{
public:
	enum CoreType
	{
		SHARED,
		HORNET,
	};
	using CorePtr = std::shared_ptr<Core::IHighLevelWrapper>;
	struct CorePkg
	{
		// The shared pointer to CardDatabase is needed so the object
		// can outlive the Core::IHighLevelWrapper that uses it.
		std::shared_ptr<CardDatabase> db;
		CorePtr core;
	};

	CoreProvider(
		std::string_view fnRegexStr,
		DataProvider& dataProvider,
		ScriptProvider& scriptProvider);
	void SetLoadProperties(CoreType typeValue, bool loadPerCallValue);

	// Will return a core instance based on the options set by SetLoadProperties
	// along with the Core::IDataSupplier used for that same core instance
	// already set as if by calling Core::IHighLevelWrapper::SetDataSupplier and
	// with the Data::ScriptProvider set as if by calling
	// Core::IHighLevelWrapper::SetScriptSupplier with `&scriptProvider`.
	CorePkg GetCorePkg();

	// IGitRepoObserver overrides
	void OnAdd(std::string_view path, const PathVector& fileList) override;
	void OnDiff(std::string_view path, const GitDiff& diff) override;

	// Core::ILogger overrides
	void Log(LogType type, std::string_view str) override;
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
