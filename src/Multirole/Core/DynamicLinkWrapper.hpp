#ifndef DYNAMICLINKWRAPPER_HPP
#define DYNAMICLINKWRAPPER_HPP
#include <string_view>
#include "IHighLevelWrapper.hpp"

namespace Ignis
{

namespace Multirole
{

namespace Core
{

class DynamicLinkWrapper : public IHighLevelWrapper
{
public:
	struct ScriptReaderData
	{
		IScriptSupplier* supplier;
		int (*OCG_LoadScript)(OCG_Duel, const char*, uint32_t, const char*);
	};

	DynamicLinkWrapper(std::string_view absFilePath);
	virtual ~DynamicLinkWrapper();

	void SetDataSupplier(IDataSupplier* ds) override;
	IDataSupplier* GetDataSupplier() override;
	void SetScriptSupplier(IScriptSupplier* ss) override;
// 	IScriptSupplier* SetScriptSupplier() override;
	void SetLogger(ILogger* l) override;
// 	ILogger* GetLogger() override;

	Duel CreateDuel(const DuelOptions& opts) override;
	void DestroyDuel(Duel duel) override;
	void AddCard(Duel duel, const OCG_NewCardInfo& info) override;
	void Start(Duel duel) override;

	DuelStatus Process(Duel duel) override;
	Buffer GetMessages(Duel duel) override;
	void SetResponse(Duel duel, const Buffer& buffer) override;

	std::size_t QueryCount(Duel duel, uint8_t team, uint32_t loc) override;
	Buffer Query(Duel duel, const QueryInfo& info) override;
	Buffer QueryLocation(Duel duel, const QueryInfo& info) override;
	Buffer QueryField(Duel duel) override;
private:
	void* handle{nullptr};

	// Function pointers from shared object
#define OCGFUNC(ret, name, args) ret (*name) args{nullptr};
#include "../../ocgapi_funcs.inl"
#undef OCGFUNC

	IDataSupplier* dataSupplier{nullptr};
	ScriptReaderData scriptReaderData{nullptr, nullptr};
	ILogger* logger{nullptr};
};

} // namespace Core

} // namespace Multirole

} // namespace Ignis

#endif // DYNAMICLINKWRAPPER_HPP
