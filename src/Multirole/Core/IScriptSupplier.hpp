#ifndef ISCRIPTSUPPLIER_HPP
#define ISCRIPTSUPPLIER_HPP
#include <string>
#include <string_view>

namespace Ignis::Multirole::Core
{

class IScriptSupplier
{
public:
	virtual std::string ScriptFromFilePath(std::string_view fp) const noexcept = 0;
protected:
	inline ~IScriptSupplier() noexcept = default;
};

} // namespace Ignis::Multirole::Core

#endif // ISCRIPTSUPPLIER_HPP
