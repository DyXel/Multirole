#ifndef ISCRIPTSUPPLIER_HPP
#define ISCRIPTSUPPLIER_HPP
#include <string>
#include <string_view>

namespace Ignis
{

namespace Multirole
{

namespace Core
{

class IScriptSupplier
{
public:
	virtual std::string ScriptFromFilePath(std::string_view fp) = 0;
};

} // namespace Core

} // namespace Multirole

} // namespace Ignis

#endif // ISCRIPTSUPPLIER_HPP
