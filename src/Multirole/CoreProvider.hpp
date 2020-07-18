#ifndef COREPROVIDER_HPP
#define COREPROVIDER_HPP
#include <regex>
#include <memory>
#include <mutex>

#include "IGitRepoObserver.hpp"

namespace Ignis::Multirole
{

namespace Core
{

class IWrapper;

} // namespace Core

class CoreProvider : public IGitRepoObserver
{
public:
	enum class CoreType
	{
		SHARED,
		HORNET,
	};

	using CorePtr = std::shared_ptr<Core::IWrapper>;

	CoreProvider(std::string_view fnRegexStr);
	void SetLoadProperties(CoreType typeValue, bool loadPerCallValue);

	// Will return a core instance based on the options set by
	// SetLoadProperties.
	CorePtr GetCore();

	// IGitRepoObserver overrides
	void OnAdd(std::string_view path, const PathVector& fileList) override;
	void OnDiff(std::string_view path, const GitDiff& diff) override;
private:
	const std::regex fnRegex;
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
