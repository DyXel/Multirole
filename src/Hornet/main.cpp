#include <string>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "../DLOpen.hpp"
#include "../HornetCommon.hpp"
#include "../ocgapi_types.h"
#include "../Read.inl"
#include "../Write.inl"

Ignis::Hornet::SharedSegment* hss;

// Shared object variables
static void* handle{nullptr};

#define OCGFUNC(ret, name, args) static ret (*name) args{nullptr};
#include "../ocgapi_funcs.inl"
#undef OCGFUNC

// Methods
[[nodiscard]] Ignis::Hornet::LockType NotifyAndWait(Ignis::Hornet::Action act)
{
	Ignis::Hornet::LockType lock(hss->mtx);
	hss->act = act;
	hss->cv.notify_one();
	hss->cv.wait(lock, [&](){return hss->act != act;});
	return lock;
}

// Used by functions that might recurse by calling callbacks
template<typename Lock, typename F, typename... Args>
inline decltype(auto) PadlockInvoke(Lock& lock, F&& f, Args&&... args)
{
	lock.unlock();
	if constexpr(std::is_same_v<std::invoke_result_t<F, Args...>, void>)
	{
		f(std::forward<Args>(args)...);
		lock.lock();
	}
	else
	{
		decltype(auto) r = f(std::forward<Args>(args)...);
		lock.lock();
		return r;
	}
}

void DataReader(void* payload, uint32_t code, OCG_CardData* data)
{
	spdlog::info("DataReader: {}", code);
	auto* wptr = hss->bytes.data();
	Write<void*>(wptr, payload);
	Write<uint32_t>(wptr, code);
	auto lock = NotifyAndWait(Ignis::Hornet::Action::CB_DATA_READER);
	std::memcpy(data, hss->bytes.data(), sizeof(OCG_CardData));
	data->setcodes = reinterpret_cast<uint16_t*>(hss->bytes.data() + sizeof(OCG_CardData));
}

int ScriptReader(void* payload, OCG_Duel duel, const char* name)
{
	spdlog::info("ScriptReader: {}", name);
	const std::size_t nameSz = std::strlen(name) + 1U;
	auto* wptr = hss->bytes.data();
	Write<void*>(wptr, payload);
	Write<std::size_t>(wptr, nameSz);
	std::memcpy(wptr, name, nameSz);
	auto lock = NotifyAndWait(Ignis::Hornet::Action::CB_SCRIPT_READER);
	const auto* rptr = hss->bytes.data();
	const auto size = Read<std::size_t>(rptr);
	if(size == 0U)
		return 0;
	const char* data = reinterpret_cast<const char*>(rptr);
	return PadlockInvoke(lock, OCG_LoadScript, duel, data, size, name);
}

void LogHandler(void* payload, const char* str, int t)
{
	spdlog::info("LogHandler: [{}] {}", t, str);
	const std::size_t strSz = std::strlen(str) + 1U;
	auto* wptr = hss->bytes.data();
	Write<void*>(wptr, payload);
	Write<int>(wptr, t);
	Write<std::size_t>(wptr, strSz);
	std::memcpy(wptr, str, strSz);
	auto lock = NotifyAndWait(Ignis::Hornet::Action::CB_LOG_HANDLER);
}

void DataReaderDone(void* payload, OCG_CardData* data)
{
	spdlog::info("DataReaderDone: {}", data->code);
	// TODO
}

int LoadSO(const char* soPath)
{
	handle = DLOpen::LoadObject(soPath);
	if(handle == nullptr)
	{
		spdlog::critical("Could not load shared object");
		return 1;
	}
#define OCGFUNC(ret, name, args) \
	do{ \
		void* funcPtr = DLOpen::LoadFunction(handle, #name); \
		(name) = reinterpret_cast<decltype(name)>(funcPtr); \
		if((name) == nullptr) \
		{ \
			DLOpen::UnloadObject(handle); \
			spdlog::critical("Could not load API function " #name); \
			return 1; \
		} \
	}while(0);
#include "../ocgapi_funcs.inl"
#undef OCGFUNC
	return 0;
}

int MainLoop(const char* shmName)
{
	using namespace Ignis::Hornet;
	try
	{
		ipc::shared_memory_object shm(ipc::open_only, shmName, ipc::read_write);
		ipc::mapped_region r(shm, ipc::read_write);
		hss = static_cast<SharedSegment*>(r.get_address());
		bool quit = false;
		do
		{
			LockType lock(hss->mtx);
			hss->cv.wait(lock, [&](){return hss->act != Action::NO_WORK;});
			switch(hss->act)
			{
#define CASE(value) case value: spdlog::info("Performing " #value);
			CASE(Action::EXIT)
			{
				quit = true;
				break;
			}
			CASE(Action::OCG_GET_VERSION)
			{
				int major, minor;
				OCG_GetVersion(&major, &minor);
				auto* wptr = hss->bytes.data();
				Write<int>(wptr, major);
				Write<int>(wptr, minor);
				break;
			}
			CASE(Action::OCG_CREATE_DUEL)
			{
				const auto* rptr = hss->bytes.data();
				auto opts = Read<OCG_DuelOptions>(rptr);
				opts.cardReader = &DataReader;
				opts.scriptReader = &ScriptReader;
				opts.logHandler = &LogHandler;
				opts.cardReaderDone = &DataReaderDone;
				OCG_Duel duel;
				int r = PadlockInvoke(lock, OCG_CreateDuel, &duel, opts);
				auto* wptr = hss->bytes.data();
				Write<int>(wptr, r);
				Write<OCG_Duel>(wptr, duel);
				break;
			}
			CASE(Action::OCG_DESTROY_DUEL)
			{
				const auto* rptr = hss->bytes.data();
				OCG_DestroyDuel(Read<OCG_Duel>(rptr));
				break;
			}
			CASE(Action::OCG_DUEL_NEW_CARD)
			{
				const auto* rptr = hss->bytes.data();
				const auto duel = Read<OCG_Duel>(rptr);
				PadlockInvoke(lock, OCG_DuelNewCard, duel, Read<OCG_NewCardInfo>(rptr));
				break;
			}
			CASE(Action::OCG_START_DUEL)
			{
				const auto* rptr = hss->bytes.data();
				OCG_StartDuel(Read<OCG_Duel>(rptr));
				break;
			}
			CASE(Action::OCG_DUEL_PROCESS)
			{
				const auto* rptr = hss->bytes.data();
				const auto duel = Read<OCG_Duel>(rptr);
				int r = PadlockInvoke(lock, OCG_DuelProcess, duel);
				auto* wptr = hss->bytes.data();
				Write<int>(wptr, r);
				break;
			}
			CASE(Action::OCG_DUEL_GET_MESSAGE)
			{
				const auto* rptr = hss->bytes.data();
				const auto duel = Read<OCG_Duel>(rptr);
				uint32_t msgLength = 0U;
				auto* msgPtr = OCG_DuelGetMessage(duel, &msgLength);
				auto* wptr = hss->bytes.data();
				Write<uint32_t>(wptr, msgLength);
				std::memcpy(wptr, msgPtr, static_cast<std::size_t>(msgLength));
				break;
			}
			CASE(Action::OCG_DUEL_SET_RESPONSE)
			{
				const auto* rptr = hss->bytes.data();
				const auto duel = Read<OCG_Duel>(rptr);
				const auto length = Read<std::size_t>(rptr);
				OCG_DuelSetResponse(duel, rptr, length);
				break;
			}
			CASE(Action::OCG_LOAD_SCRIPT)
			{
				const auto* rptr = hss->bytes.data();
				const auto duel = Read<OCG_Duel>(rptr);
				const auto nameSize = Read<std::size_t>(rptr);
				const auto* name = reinterpret_cast<const char*>(rptr);
				rptr += nameSize;
				const auto strSize = Read<std::size_t>(rptr);
				const auto* str = reinterpret_cast<const char*>(rptr);
				int r = PadlockInvoke(lock, OCG_LoadScript, duel, str, strSize, name);
				auto* wptr = hss->bytes.data();
				Write<int>(wptr, r);
				break;
			}
			CASE(Action::OCG_DUEL_QUERY_COUNT)
			{
				const auto* rptr = hss->bytes.data();
				const auto duel = Read<OCG_Duel>(rptr);
				const auto team = Read<uint8_t>(rptr);
				const auto loc = Read<uint32_t>(rptr);
				auto* wptr = hss->bytes.data();
				Write<uint32_t>(wptr, OCG_DuelQueryCount(duel, team, loc));
				break;
			}
			CASE(Action::OCG_DUEL_QUERY)
			{
				const auto* rptr = hss->bytes.data();
				const auto duel = Read<OCG_Duel>(rptr);
				const auto info = Read<OCG_QueryInfo>(rptr);
				uint32_t qLength = 0U;
				auto* qPtr = OCG_DuelQuery(duel, &qLength, info);
				auto* wptr = hss->bytes.data();
				Write<uint32_t>(wptr, qLength);
				std::memcpy(wptr, qPtr, static_cast<std::size_t>(qLength));
				break;
			}
			CASE(Action::OCG_DUEL_QUERY_LOCATION)
			{
				const auto* rptr = hss->bytes.data();
				const auto duel = Read<OCG_Duel>(rptr);
				const auto info = Read<OCG_QueryInfo>(rptr);
				uint32_t qLength = 0U;
				auto* qPtr = OCG_DuelQueryLocation(duel, &qLength, info);
				auto* wptr = hss->bytes.data();
				Write<uint32_t>(wptr, qLength);
				std::memcpy(wptr, qPtr, static_cast<std::size_t>(qLength));
				break;
			}
			CASE(Action::OCG_DUEL_QUERY_FIELD)
			{
				const auto* rptr = hss->bytes.data();
				const auto duel = Read<OCG_Duel>(rptr);
				uint32_t qLength = 0U;
				auto* qPtr = OCG_DuelQueryField(duel, &qLength);
				auto* wptr = hss->bytes.data();
				Write<uint32_t>(wptr, qLength);
				std::memcpy(wptr, qPtr, static_cast<std::size_t>(qLength));
				break;
			}
#undef CASE
			case Action::NO_WORK:
			case Action::CB_DATA_READER
			case Action::CB_SCRIPT_READER:
			case Action::CB_LOG_HANDLER:
			case Action::CB_DATA_READER_DONE:
			case Action::CB_DONE:
				break;
			}
			hss->act = Action::NO_WORK;
			hss->cv.notify_one();
		}while(!quit);
	}
	catch(const ipc::interprocess_exception& e)
	{
		spdlog::critical(e.what());
		return 1;
	}
	return 0;
}

int main(int argc, char* argv[])
{
	if(argc < 3)
		return 1;
	// Setup spdlog
	{
		std::string logName(argv[2]);
		logName += ".log";
		using namespace spdlog;
		auto logger = basic_logger_st("file_logger", logName.data());
		logger->flush_on(level::info);
		set_default_logger(std::move(logger));
		{
			std::string args(argv[0]);
			for(int i = 1; i < argc; i++)
				args.append(" ").append(argv[i]);
			spdlog::info("launched with args: {}", args);
		}
	}
	if(int r = LoadSO(argv[1]); r != 0)
	{
		spdlog::shutdown();
		return r;
	}
	spdlog::info("Shared object loaded");
	int exitFlag = MainLoop(argv[2]);
	DLOpen::UnloadObject(handle);
	spdlog::shutdown();
	return exitFlag;
}
