#ifndef IGITREPOOBSERVER_HPP
#define IGITREPOOBSERVER_HPP

namespace Ignis
{

namespace Multirole
{

class IGitRepoObserver
{
public:
	virtual void OnAdd(/*std::vector<std::string> fullFileList*/) = 0;
	virtual void OnReset(std::vector<std::string> deltaFileList) = 0;
};

} // namespace Multirole

} // namespace Ignis

#endif // IGITREPOOBSERVER_HPP
