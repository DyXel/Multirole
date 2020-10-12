#ifndef PROCESS_HPP
#define PROCESS_HPP

namespace Process
{

template<typename... Args>
void Launch(const char* program, Args&& ...args);

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Detail
{

inline void AppendArgs([[maybe_unused]] std::string& out)
{}

template<typename T, typename... Args>
void AppendArgs(std::string& out, T&& arg, Args&& ...args)
{
	out.append(" \"").append(arg).append("\"");
	AppendArgs(out, std::forward<Args>(args)...);
}

} // namespace Detail

template<typename... Args>
void Launch(const char* program, Args&& ...args)
{
	std::string param(program);
	Detail::AppendArgs(param, std::forward<Args>(args)...);
	STARTUPINFOA si{};
	PROCESS_INFORMATION pi{};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	if(CreateProcessA(nullptr, param.data(), nullptr, nullptr,
	   FALSE, 0, nullptr, nullptr, &si, &pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}

#else
#include <sys/types.h>
#include <unistd.h>

template<typename... Args>
void Launch(const char* program, Args&& ...args)
{
	if(vfork() != 0)
		return;
	constexpr const char* NULL_CHAR_PTR = nullptr;
	execlp(program, program, std::forward<Args>(args)..., NULL_CHAR_PTR);
}

#endif // _WIN32

} // namespace Process

#endif // PROCESS_HPP
