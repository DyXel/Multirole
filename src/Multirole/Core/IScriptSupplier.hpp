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
	virtual std::string ScriptFromFilePath(std::string_view fp) const = 0;
protected:
	inline ~IScriptSupplier() = default;
};

} // namespace Core

} // namespace Multirole

} // namespace Ignis

#endif // ISCRIPTSUPPLIER_HPP
