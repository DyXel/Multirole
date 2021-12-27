#ifndef ISCRIPTSUPPLIER_HPP
#define ISCRIPTSUPPLIER_HPP
#include <memory>
#include <string>
#include <string_view>

namespace Ignis::Multirole::Core
{

class IScriptSupplier
{
public:
	using ScriptType = std::shared_ptr<const std::string>;

	static inline const char* GetData(const ScriptType& script) noexcept
	{
		if(!script || script->empty())
			return nullptr;
		return script->data();
	}

	static inline std::size_t GetSize(const ScriptType& script) noexcept
	{
		if(!script)
			return 0;
		return script->size();
	}

	virtual ScriptType ScriptFromFilePath(std::string_view fp) const noexcept = 0;
protected:
	inline ~IScriptSupplier() noexcept = default;
};

} // namespace Ignis::Multirole::Core

#endif // ISCRIPTSUPPLIER_HPP
