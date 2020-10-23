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

using LockType = ipc::scoped_lock<ipc::interprocess_mutex>;

// Interprocess variables
Ignis::Hornet::SharedSegment* hss;

// Shared object variables
static void* handle{nullptr};

#define OCGFUNC(ret, name, args) static ret (*name) args{nullptr};
#include "../ocgapi_funcs.inl"
#undef OCGFUNC

// Core callbacks
void DataReader(void* payload, uint32_t code, OCG_CardData* data);
int ScriptReader(void* payload, OCG_Duel duel, const char* name);
void LogHandler(void* payload, const char* str, int t);
void DataReaderDone(void* payload, OCG_CardData* data);

// Other functions
int LoadSO(const char* soPath);
int MainLoop(const char* shmName);

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
			CASE(Action::OCG_GET_VERSION)
			{
				int major, minor;
				OCG_GetVersion(&major, &minor);
				auto* wptr = hss->bytes.data();
				Write<int>(wptr, major);
				Write<int>(wptr, minor);
				break;
			}
			CASE(Action::OCG_DESTROY_DUEL)
			{
				const auto* rptr = hss->bytes.data();
				OCG_DestroyDuel(Read<OCG_Duel>(rptr));
				break;
			}
			CASE(Action::OCG_START_DUEL)
			{
				const auto* rptr = hss->bytes.data();
				OCG_StartDuel(Read<OCG_Duel>(rptr));
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
			default: // TODO: remove, every case should be handled.
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
