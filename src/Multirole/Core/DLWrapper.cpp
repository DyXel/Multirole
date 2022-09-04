#include "DLWrapper.hpp"

#include <cstring> // std::memcpy

#include "IDataSupplier.hpp"
#include "IScriptSupplier.hpp"
#include "ILogger.hpp"
#include "../I18N.hpp"
#include "../../DLOpen.hpp"

namespace Ignis::Multirole::Core
{

namespace
{

// Core callbacks
static void DataReader(void* payload, uint32_t code, OCG_CardData* data)
{
	*data = static_cast<IDataSupplier*>(payload)->DataFromCode(code);
}

static int ScriptReader(void* payload, OCG_Duel duel, const char* name)
{
	auto& ssd = *static_cast<Detail::ScriptSupplierData*>(payload);
	const auto script = ssd.supplier.ScriptFromFilePath(name);
	const char* const data = IScriptSupplier::GetData(script);
	if(data == nullptr)
		return 0;
	return ssd.OCG_LoadScript(duel, data, IScriptSupplier::GetSize(script), name);
}

static void LogHandler(void* payload, const char* str, int t)
{
	if(payload != nullptr)
		static_cast<ILogger*>(payload)->Log(ILogger::LogType{t}, str);
}

static void DataReaderDone(void* payload, OCG_CardData* data)
{
	static_cast<IDataSupplier*>(payload)->DataUsageDone(*data);
}

} // namespace

// public

DLWrapper::DLWrapper(std::string_view absFilePath)
{
	handle = DLOpen::LoadObject(absFilePath.data());
	// Load every function from the shared object into the functions
	try
	{
#define OCGFUNC(ret, name, args) \
		void* name##VoidPtr = DLOpen::LoadFunction(handle, #name); \
		(name) = reinterpret_cast<decltype(name)>(name##VoidPtr);
#include "../../ocgapi_funcs.inl"
#undef OCGFUNC
	}
	catch(std::runtime_error& e)
	{
		DLOpen::UnloadObject(handle);
		throw;
	}
}

DLWrapper::~DLWrapper()
{
	DLOpen::UnloadObject(handle);
}

std::pair<int, int> DLWrapper::Version()
{
	std::pair<int, int> p;
	OCG_GetVersion(&p.first, &p.second);
	return p;
}

IWrapper::Duel DLWrapper::CreateDuel(const DuelOptions& opts)
{
	OCG_Duel duel{nullptr};
	std::scoped_lock lock(ssdMutex);
	auto ssdIter = ssdList.insert(ssdList.end(), {opts.scriptSupplier, OCG_LoadScript});
	OCG_DuelOptions options =
	{
		{opts.seed[0U], opts.seed[1U], opts.seed[2U], opts.seed[3U]},
		opts.flags,
		opts.team1,
		opts.team2,
		&DataReader,
		&opts.dataSupplier,
		&ScriptReader,
		&*ssdIter,
		&LogHandler,
		opts.optLogger,
		&DataReaderDone,
		&opts.dataSupplier,
		0
	};
	if(OCG_CreateDuel(&duel, options) != OCG_DUEL_CREATION_SUCCESS)
	{
		ssdList.erase(ssdIter);
		throw Core::Exception(I18N::DLWRAPPER_EXCEPT_CREATE_DUEL);
	}
	ssdMap.insert({duel, ssdIter});
	return duel;
}

void DLWrapper::DestroyDuel(Duel duel)
{
	{
		std::scoped_lock lock(ssdMutex);
		ssdMap.erase(duel);
	}
	OCG_DestroyDuel(duel);
}

void DLWrapper::AddCard(Duel duel, const OCG_NewCardInfo& info)
{
	OCG_DuelNewCard(duel, info);
}

void DLWrapper::Start(Duel duel)
{
	OCG_StartDuel(duel);
}

IWrapper::DuelStatus DLWrapper::Process(Duel duel)
{
	return DuelStatus{OCG_DuelProcess(duel)};
}

IWrapper::Buffer DLWrapper::GetMessages(Duel duel)
{
	uint32_t length = 0U;
	auto* pointer = OCG_DuelGetMessage(duel, &length);
	Buffer buffer(static_cast<Buffer::size_type>(length));
	std::memcpy(buffer.data(), pointer, static_cast<std::size_t>(length));
	return buffer;
}

void DLWrapper::SetResponse(Duel duel, const Buffer& buffer)
{
	OCG_DuelSetResponse(duel, buffer.data(), buffer.size());
}

int DLWrapper::LoadScript(Duel duel, std::string_view name, std::string_view str)
{
	return OCG_LoadScript(duel, str.data(), str.size(), name.data());
}

std::size_t DLWrapper::QueryCount(Duel duel, uint8_t team, uint32_t loc)
{
	return static_cast<std::size_t>(OCG_DuelQueryCount(duel, team, loc));
}

IWrapper::Buffer DLWrapper::Query(Duel duel, const QueryInfo& info)
{
	uint32_t length = 0U;
	auto* pointer = OCG_DuelQuery(duel, &length, info);
	Buffer buffer(static_cast<Buffer::size_type>(length));
	std::memcpy(buffer.data(), pointer, static_cast<std::size_t>(length));
	return buffer;
}

IWrapper::Buffer DLWrapper::QueryLocation(Duel duel, const QueryInfo& info)
{
	uint32_t length = 0U;
	auto* pointer = OCG_DuelQueryLocation(duel, &length, info);
	Buffer buffer(static_cast<Buffer::size_type>(length));
	std::memcpy(buffer.data(), pointer, static_cast<std::size_t>(length));
	return buffer;
}

IWrapper::Buffer DLWrapper::QueryField(Duel duel)
{
	uint32_t length = 0;
	auto* pointer = OCG_DuelQueryField(duel, &length);
	Buffer buffer(static_cast<Buffer::size_type>(length));
	std::memcpy(buffer.data(), pointer, static_cast<std::size_t>(length));
	return buffer;
}

} // namespace Ignis::Multirole::Core
