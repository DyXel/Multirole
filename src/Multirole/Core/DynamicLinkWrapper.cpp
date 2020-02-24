#include "DynamicLinkWrapper.hpp"

#include <cstring>
#include <stdexcept> // std::runtime_error

#include "IDataSupplier.hpp"
#include "IScriptSupplier.hpp"
#include "ILogger.hpp"
#include "../../DLOpen.hpp"

namespace Ignis
{

namespace Multirole
{

namespace Core
{

// Core callbacks
static void DataReader(void* payload, int code, OCG_CardData* data)
{
	*data = static_cast<IDataSupplier*>(payload)->DataFromCode(code);
}

static int ScriptReader(void* payload, OCG_Duel duel, const char* name)
{
	auto srd = static_cast<DynamicLinkWrapper::ScriptReaderData*>(payload);
	std::string script = srd->supplier->ScriptFromFilePath(name);
	if(script.empty())
		return 0;
	return srd->OCG_LoadScript(duel, script.data(), script.length(), name);
}

static void LogHandler(void* payload, const char* str, int t)
{
	static_cast<ILogger*>(payload)->Log(static_cast<ILogger::LogType>(t), str);
}

void DataReaderDone(void* payload, OCG_CardData* data)
{
	static_cast<IDataSupplier*>(payload)->DataUsageDone(*data);
}

// public

DynamicLinkWrapper::DynamicLinkWrapper(std::string_view absFilePath)
{
	handle = DLOpen::LoadObject(absFilePath.data());
	if(handle == nullptr)
		throw std::runtime_error("Could not load core.");
	// Load every function from the shared object into the functions
#define OCGFUNC(ret, name, args) \
	do{ \
	void* funcPtr = DLOpen::LoadFunction(handle, #name); \
	(name) = reinterpret_cast<decltype(name)>(funcPtr); \
	if(name == nullptr) \
	{ \
		DLOpen::UnloadObject(handle); \
		throw std::runtime_error("Could not load API function."); \
	} \
	}while(0);
#include "../../ocgapi_funcs.inl"
#undef OCGFUNC
	scriptReaderData.OCG_LoadScript = OCG_LoadScript;
}

DynamicLinkWrapper::~DynamicLinkWrapper()
{
	DLOpen::UnloadObject(handle);
}

void DynamicLinkWrapper::SetDataSupplier(IDataSupplier* ds)
{
	dataSupplier = ds;
}

IDataSupplier* DynamicLinkWrapper::GetDataSupplier()
{
	return dataSupplier;
}

void DynamicLinkWrapper::SetScriptSupplier(IScriptSupplier* ss)
{
	scriptReaderData.supplier = ss;
}

void DynamicLinkWrapper::SetLogger(ILogger* l)
{
	logger = l;
}

IHighLevelWrapper::Duel DynamicLinkWrapper::CreateDuel(const DuelOptions& opts)
{
	OCG_DuelOptions options =
	{
		opts.seed,
		opts.flags,
		opts.team1,
		opts.team2,
		&DataReader,
		dataSupplier,
		&ScriptReader,
		&scriptReaderData,
		&LogHandler,
		logger,
		&DataReaderDone,
		dataSupplier
	};
	OCG_Duel duel = nullptr;
	if(OCG_CreateDuel(&duel, options) != OCG_DUEL_CREATION_SUCCESS)
		throw std::runtime_error("Could not create duel");
	return duel;
}

void DynamicLinkWrapper::DestroyDuel(Duel duel)
{
	OCG_DestroyDuel(duel);
}

void DynamicLinkWrapper::AddCard(Duel duel, const OCG_NewCardInfo& info)
{
	OCG_DuelNewCard(duel, info);
}

void DynamicLinkWrapper::Start(Duel duel)
{
	OCG_StartDuel(duel);
}

IHighLevelWrapper::DuelStatus DynamicLinkWrapper::Process(Duel duel)
{
	return static_cast<DuelStatus>(OCG_DuelProcess(duel));
}

IHighLevelWrapper::Buffer DynamicLinkWrapper::GetMessages(Duel duel)
{
	uint32_t length;
	auto pointer = OCG_DuelGetMessage(duel, &length);
	Buffer buffer(static_cast<Buffer::size_type>(length));
	std::memcpy(buffer.data(), pointer, static_cast<std::size_t>(length));
	return buffer;
}

void DynamicLinkWrapper::SetResponse(Duel duel, const Buffer& buffer)
{
	OCG_DuelSetResponse(duel, buffer.data(), buffer.size());
}

std::size_t DynamicLinkWrapper::QueryCount(Duel duel, uint8_t team, uint32_t loc)
{
	return static_cast<std::size_t>(OCG_DuelQueryCount(duel, team, loc));
}

IHighLevelWrapper::Buffer DynamicLinkWrapper::Query(Duel duel, const QueryInfo& info)
{
	uint32_t length;
	auto pointer = OCG_DuelQuery(duel, &length, info);
	Buffer buffer(static_cast<Buffer::size_type>(length));
	std::memcpy(buffer.data(), pointer, static_cast<std::size_t>(length));
	return buffer;
}

IHighLevelWrapper::Buffer DynamicLinkWrapper::QueryLocation(Duel duel, const QueryInfo& info)
{
	uint32_t length;
	auto pointer = OCG_DuelQueryLocation(duel, &length, info);
	Buffer buffer(static_cast<Buffer::size_type>(length));
	std::memcpy(buffer.data(), pointer, static_cast<std::size_t>(length));
	return buffer;
}

IHighLevelWrapper::Buffer DynamicLinkWrapper::QueryField(Duel duel)
{
	uint32_t length;
	auto pointer = OCG_DuelQueryField(duel, &length);
	Buffer buffer(static_cast<Buffer::size_type>(length));
	std::memcpy(buffer.data(), pointer, static_cast<std::size_t>(length));
	return buffer;
}

} // namespace Core

} // namespace Multirole

} // namespace Ignis
