#ifndef SERVICE_COREPROVIDER_HPP
#define SERVICE_COREPROVIDER_HPP
#include "../Service.hpp"

#include <chrono>
#include <regex>
#include <memory>
#include <shared_mutex>

#include "../IGitRepoObserver.hpp"

namespace Ignis::Multirole
{

namespace Core
{

class IWrapper;

} // namespace Core

class Service::CoreProvider final : public IGitRepoObserver
{
public:
	enum class CoreType
	{
		SHARED,
		HORNET,
	};

	using CorePtr = std::shared_ptr<Core::IWrapper>;

	CoreProvider(std::string_view fnRegexStr, std::string_view tmpPath, CoreType type, bool loadPerCall);
	~CoreProvider();

	// Will return a core instance based on the options set.
	CorePtr GetCore() const;

	// IGitRepoObserver overrides
	void OnAdd(std::string_view path, const PathVector& fileList) override;
	void OnDiff(std::string_view path, const GitDiff& diff) override;
private:
	const std::regex fnRegex;
	const std::string tmpPath;
	const CoreType type;
	const bool loadPerCall;
	const std::chrono::system_clock::rep uniqueId;
	std::size_t loadCount;
	bool shouldTest;
	std::string corePath;
	CorePtr core;
	mutable std::shared_mutex mCore; // used for both corePath and core.

	CorePtr LoadCore() const;

	void OnGitUpdate(std::string_view path, const PathVector& fl);
};

} // namespace Ignis::Multirole

#endif // SERVICE_COREPROVIDER_HPP
