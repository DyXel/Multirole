#ifndef IGITREPOOBSERVER_HPP
#define IGITREPOOBSERVER_HPP
#include <vector>

#include <filesystem>

namespace Ignis::Multirole
{

using PathVector = std::vector<std::filesystem::path>;

struct GitDiff
{
	// NOTE: filenames that were only modified will be present in both vectors
	PathVector removed;
	PathVector added;
};

class IGitRepoObserver
{
public:
	virtual void OnAdd(const std::filesystem::path& path, const PathVector& fileList) = 0;
	virtual void OnDiff(const std::filesystem::path& path, const GitDiff& diff) = 0;
protected:
	inline ~IGitRepoObserver() noexcept = default;
};

} // namespace Ignis::Multirole

#endif // IGITREPOOBSERVER_HPP
