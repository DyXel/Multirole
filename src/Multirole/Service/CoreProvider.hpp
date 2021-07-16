#ifndef SERVICE_COREPROVIDER_HPP
#define SERVICE_COREPROVIDER_HPP
#include "../Service.hpp"

#include <chrono>
#include <list>
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

	CoreProvider(Service::LogHandler& lh, std::string_view fnRegexStr, const boost::filesystem::path& tmpDir, CoreType type, bool loadPerCall);
	~CoreProvider() noexcept;

	// Will return a core instance based on the options set.
	CorePtr GetCore() const;

	// IGitRepoObserver overrides
	void OnAdd(const boost::filesystem::path& path, const PathVector& fileList) override;
	void OnDiff(const boost::filesystem::path& path, const GitDiff& diff) override;
private:
	Service::LogHandler& lh;
	const std::regex fnRegex;
	const boost::filesystem::path tmpDir;
	const CoreType type;
	const bool loadPerCall;
	const std::chrono::system_clock::rep uniqueId;
	std::size_t loadCount;
	bool shouldTest;
	boost::filesystem::path coreLoc;
	CorePtr core;
	std::list<boost::filesystem::path> pLocs; // Previous locations for core file.
	mutable std::shared_mutex mCore; // used for both corePath and core.

	CorePtr LoadCore() const;

	void OnGitUpdate(const boost::filesystem::path& path, const PathVector& fileList);
};

} // namespace Ignis::Multirole

#endif // SERVICE_COREPROVIDER_HPP
