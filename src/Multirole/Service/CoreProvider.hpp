#ifndef SERVICE_COREPROVIDER_HPP
#define SERVICE_COREPROVIDER_HPP
#include "../Service.hpp"

#include <array>
#include <chrono>
#include <list>
#include <regex>
#include <memory>
#include <shared_mutex>
#include <mutex>

#include <boost/random/random_device.hpp>

#include "../IGitRepoObserver.hpp"
#include "../Core/IWrapper.hpp"

namespace Ignis::Multirole
{

class Service::CoreProvider final : public IGitRepoObserver
{
public:
	enum class CoreType
	{
		SHARED,
		HORNET,
	};

	using CorePtr = std::shared_ptr<Core::IWrapper>;

	CoreProvider(Service::LogHandler& lh, std::string_view fnRegexStr, const std::filesystem::path& tmpDir, CoreType type, bool loadPerCall);
	~CoreProvider() noexcept;

	// Generate a random seed suitable for use with shufflers and new duels.
	Core::IWrapper::DuelOptions::SeedType GenSeed() noexcept;

	// Will return a core instance based on the options set.
	CorePtr GetCore() const;

	// IGitRepoObserver overrides
	void OnAdd(const std::filesystem::path& path, const PathVector& fileList) override;
	void OnDiff(const std::filesystem::path& path, const GitDiff& diff) override;
private:
	Service::LogHandler& lh;
	const std::regex fnRegex;
	const std::filesystem::path tmpDir;
	const CoreType type;
	const bool loadPerCall;
	const std::chrono::system_clock::rep uniqueId;
	std::size_t loadCount;
	bool shouldTest;
	std::filesystem::path coreLoc;
	CorePtr core;
	std::list<std::filesystem::path> pLocs; // Previous locations for core file.
	mutable std::shared_mutex mCore; // used for both corePath and core.
	boost::random::random_device rng;
	std::mutex mRng; // used for RNG access.

	CorePtr LoadCore() const;

	void OnGitUpdate(const std::filesystem::path& path, const PathVector& fileList);
};

} // namespace Ignis::Multirole

#endif // SERVICE_COREPROVIDER_HPP
