#ifndef DLWRAPPER_HPP
#define DLWRAPPER_HPP
#include <list>
#include <map>
#include <mutex>
#include "IWrapper.hpp"

namespace Ignis::Multirole::Core
{

namespace Detail
{

struct ScriptSupplierData
{
	IScriptSupplier& supplier;
	int (*OCG_LoadScript)(OCG_Duel, const char*, uint32_t, const char*);
};

} // namespace Detail

class DLWrapper final : public IWrapper
{
public:
	DLWrapper(std::string_view absFilePath);
	~DLWrapper();

	std::pair<int, int> Version() override;

	Duel CreateDuel(const DuelOptions& opts) override;
	void DestroyDuel(Duel duel) override;
	void AddCard(Duel duel, const NewCardInfo& info) override;
	void Start(Duel duel) override;

	DuelStatus Process(Duel duel) override;
	Buffer GetMessages(Duel duel) override;
	void SetResponse(Duel duel, const Buffer& buffer) override;
	int LoadScript(Duel duel, std::string_view name, std::string_view str) override;

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

	// The script supplier core callback must know which OCG_LoadScript
	// function to call in order to pass the correct data to the core,
	// this is achieved by saving both the script supplier as well as the
	// function pointer in the same struct. We own that information through
	// the list, but since the list has no associated duel, we must also
	// save the iterator in a map when we actually get the necessary key,
	// in this case, the duel pointer after duel creation has succeeded.
	std::list<Detail::ScriptSupplierData> ssdList;
	std::map<Duel, std::list<Detail::ScriptSupplierData>::iterator> ssdMap;
	std::mutex ssdMutex;
};

} // namespace Ignis::Multirole::Core

#endif // DLWRAPPER_HPP
