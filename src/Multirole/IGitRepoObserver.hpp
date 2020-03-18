#ifndef IGITREPOOBSERVER_HPP
#define IGITREPOOBSERVER_HPP
#include <string>
#include <string_view>
#include <vector>

namespace Ignis::Multirole
{

using PathVector = std::vector<std::string>;

struct GitDiff
{
	// NOTE: filenames that were only modified will be present in both vectors
	PathVector removed;
	PathVector added;
};

class IGitRepoObserver
{
public:
	virtual void OnAdd(std::string_view path, const PathVector& fileList) = 0;
	virtual void OnDiff(std::string_view path, const GitDiff& diff) = 0;
};

} // namespace Ignis::Multirole

#endif // IGITREPOOBSERVER_HPP
