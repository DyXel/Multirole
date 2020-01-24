#ifndef IASYNCLOGGER_HPP
#define IASYNCLOGGER_HPP
#include <string_view>

namespace Ignis
{

namespace Multirole
{

class IAsyncLogger
{
public:
	virtual void Log(std::string_view str) = 0;
	virtual void LogError(std::string_view str) = 0;
};

} // namespace Multirole

} // namespace Ignis

#endif // IASYNCLOGGER_HPP
