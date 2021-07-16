#ifndef IGITREPOOBSERVER_HPP
#define IGITREPOOBSERVER_HPP
#include <vector>

#include <boost/filesystem/path.hpp>

namespace Ignis::Multirole
{

using PathVector = std::vector<boost::filesystem::path>;

struct GitDiff
{
	// NOTE: filenames that were only modified will be present in both vectors
	PathVector removed;
	PathVector added;
};

class IGitRepoObserver
{
public:
	virtual void OnAdd(const boost::filesystem::path& path, const PathVector& fileList) = 0;
	virtual void OnDiff(const boost::filesystem::path& path, const GitDiff& diff) = 0;
protected:
	inline ~IGitRepoObserver() noexcept = default;
};

} // namespace Ignis::Multirole

#endif // IGITREPOOBSERVER_HPP
