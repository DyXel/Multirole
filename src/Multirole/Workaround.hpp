#ifndef MULTIROLE_WORKAROUND_HPP
#define MULTIROLE_WORKAROUND_HPP

namespace Ignis::Multirole::Workaround
{

#ifdef _WIN32
template<typename NativeHandle>
inline void SetCloseOnExec([[maybe_unused]]NativeHandle handle)
{}
#else
#include <fcntl.h>
inline void SetCloseOnExec(int fd)
{
	fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
}
#endif // _WIN32

} // namespace Ignis::Multirole::Workaround

#endif // MULTIROLE_WORKAROUND_HPP
