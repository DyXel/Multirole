#ifdef __linux__
#include <sys/prctl.h> // prctl(), PR_SET_PDEATHSIG
#endif // __linux__

#ifndef _WIN32
#include <csignal>
#include <cstdlib>
#endif // _WIN32

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "../DLOpen.hpp"
#include "../HornetCommon.hpp"
#include "../ocgapi_types.h"
#include "../Read.inl"
#include "../Write.inl"

static Ignis::Hornet::SharedSegment* hss{nullptr};

// Shared object variables
static void* handle{nullptr};

#define OCGFUNC(ret, name, args) static ret (*name) args{nullptr};
#include "../ocgapi_funcs.inl"
#undef OCGFUNC

// Methods
void NotifyAndWait(Ignis::Hornet::Action act)
{
	auto recvAct = Ignis::Hornet::Action::NO_WORK;
	{
		Ignis::Hornet::LockType lock(hss->mtx);
		hss->act = act;
		hss->cv.notify_one();
		hss->cv.wait(lock, [&](){return hss->act != act;});
		recvAct = hss->act;
	}
	// The only scenario where this would not be CB_DONE is when Multirole was
	// trying to signal us to quit and we were unresponsive. We have to
	// terminate to guarantee that the resources will not be in usage when the
	// shared segment is destroyed.
	if(recvAct != Ignis::Hornet::Action::CB_DONE)
		std::terminate();
}

void DataReader(void* payload, uint32_t code, OCG_CardData* data)
{
	auto* wptr = hss->bytes.data();
	Write<void*>(wptr, payload);
	Write<uint32_t>(wptr, code);
	NotifyAndWait(Ignis::Hornet::Action::CB_DATA_READER);
	std::memcpy(data, hss->bytes.data(), sizeof(OCG_CardData));
	data->setcodes = reinterpret_cast<uint16_t*>(hss->bytes.data() + sizeof(OCG_CardData));
}

int ScriptReader(void* payload, OCG_Duel duel, const char* name)
{
	const std::size_t nameSz = std::strlen(name) + 1U;
	auto* wptr = hss->bytes.data();
	Write<void*>(wptr, payload);
	Write<std::size_t>(wptr, nameSz);
	std::memcpy(wptr, name, nameSz);
	NotifyAndWait(Ignis::Hornet::Action::CB_SCRIPT_READER);
	const auto* rptr = hss->bytes.data();
	const auto size = Read<std::size_t>(rptr);
	if(size == 0U)
		return 0;
	const char* data = reinterpret_cast<const char*>(rptr);
	return OCG_LoadScript(duel, data, size, name);
}

void LogHandler(void* payload, const char* str, int t)
{
	const std::size_t strSz = std::strlen(str) + 1U;
	auto* wptr = hss->bytes.data();
	Write<void*>(wptr, payload);
	Write<int>(wptr, t);
	Write<std::size_t>(wptr, strSz);
	std::memcpy(wptr, str, strSz);
	NotifyAndWait(Ignis::Hornet::Action::CB_LOG_HANDLER);
}

void DataReaderDone(void* payload, OCG_CardData* data)
{
	auto* wptr = hss->bytes.data();
	Write<void*>(wptr, payload);
	std::memcpy(wptr, data, sizeof(OCG_CardData));
	NotifyAndWait(Ignis::Hornet::Action::CB_DATA_READER_DONE);
}

int LoadSO(const char* soPath)
{
	handle = DLOpen::LoadObject(soPath);
	if(handle == nullptr)
		return 1;
	try
	{
#define OCGFUNC(ret, name, args) \
		void* name##VoidPtr = DLOpen::LoadFunction(handle, #name); \
		(name) = reinterpret_cast<decltype(name)>(name##VoidPtr);
#include "../ocgapi_funcs.inl"
#undef OCGFUNC
	}
	catch(std::runtime_error& e)
	{
		DLOpen::UnloadObject(handle);
		return 1;
	}
	return 0;
}

void MainLoop()
{
	using namespace Ignis::Hornet;
	for(;;)
	{
		{
			LockType lock(hss->mtx);
			hss->act = Action::NO_WORK;
			hss->cv.notify_one();
			hss->cv.wait(lock, [&](){return hss->act != Action::NO_WORK;});
		}
		switch(hss->act)
		{
		case Action::EXIT:
		{
			Ignis::Hornet::LockType lock(hss->mtx);
			hss->act = Action::EXIT_CONFIRMED;
			hss->cv.notify_one();
			return; // NOTE: Returning, not breaking!
		}
		case Action::OCG_GET_VERSION:
		{
			int major = 0;
			int minor = 0;
			OCG_GetVersion(&major, &minor);
			auto* wptr = hss->bytes.data();
			Write<int>(wptr, major);
			Write<int>(wptr, minor);
			break;
		}
		case Action::OCG_CREATE_DUEL:
		{
			const auto* rptr = hss->bytes.data();
			auto opts = Read<OCG_DuelOptions>(rptr);
			opts.cardReader = &DataReader;
			opts.scriptReader = &ScriptReader;
			opts.logHandler = &LogHandler;
			opts.cardReaderDone = &DataReaderDone;
			OCG_Duel duel = nullptr;
			int r = OCG_CreateDuel(&duel, opts);
			auto* wptr = hss->bytes.data();
			Write<int>(wptr, r);
			Write<OCG_Duel>(wptr, duel);
			break;
		}
		case Action::OCG_DESTROY_DUEL:
		{
			const auto* rptr = hss->bytes.data();
			OCG_DestroyDuel(Read<OCG_Duel>(rptr));
			break;
		}
		case Action::OCG_DUEL_NEW_CARD:
		{
			const auto* rptr = hss->bytes.data();
			const auto duel = Read<OCG_Duel>(rptr);
			OCG_DuelNewCard(duel, Read<OCG_NewCardInfo>(rptr));
			break;
		}
		case Action::OCG_START_DUEL:
		{
			const auto* rptr = hss->bytes.data();
			OCG_StartDuel(Read<OCG_Duel>(rptr));
			break;
		}
		case Action::OCG_DUEL_PROCESS:
		{
			const auto* rptr = hss->bytes.data();
			const auto duel = Read<OCG_Duel>(rptr);
			int r = OCG_DuelProcess(duel);
			auto* wptr = hss->bytes.data();
			Write<int>(wptr, r);
			break;
		}
		case Action::OCG_DUEL_GET_MESSAGE:
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
		case Action::OCG_DUEL_SET_RESPONSE:
		{
			const auto* rptr = hss->bytes.data();
			const auto duel = Read<OCG_Duel>(rptr);
			const auto length = Read<std::size_t>(rptr);
			OCG_DuelSetResponse(duel, rptr, length);
			break;
		}
		case Action::OCG_LOAD_SCRIPT:
		{
			const auto* rptr = hss->bytes.data();
			const auto duel = Read<OCG_Duel>(rptr);
			const auto nameSize = Read<std::size_t>(rptr);
			const auto* name = reinterpret_cast<const char*>(rptr);
			rptr += nameSize;
			const auto strSize = Read<std::size_t>(rptr);
			const auto* str = reinterpret_cast<const char*>(rptr);
			int r = OCG_LoadScript(duel, str, strSize, name);
			auto* wptr = hss->bytes.data();
			Write<int>(wptr, r);
			break;
		}
		case Action::OCG_DUEL_QUERY_COUNT:
		{
			const auto* rptr = hss->bytes.data();
			const auto duel = Read<OCG_Duel>(rptr);
			const auto team = Read<uint8_t>(rptr);
			const auto loc = Read<uint32_t>(rptr);
			auto* wptr = hss->bytes.data();
			Write<uint32_t>(wptr, OCG_DuelQueryCount(duel, team, loc));
			break;
		}
		case Action::OCG_DUEL_QUERY:
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
		case Action::OCG_DUEL_QUERY_LOCATION:
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
		case Action::OCG_DUEL_QUERY_FIELD:
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
		// Explicitly ignore these, in case we ever add more functionality...
		case Action::NO_WORK:
		case Action::HEARTBEAT:
		case Action::EXIT_CONFIRMED:
		case Action::CB_DATA_READER:
		case Action::CB_SCRIPT_READER:
		case Action::CB_LOG_HANDLER:
		case Action::CB_DATA_READER_DONE:
		case Action::CB_DONE:
			break;
		}
	}
}

int main(int argc, char* argv[])
{
	if(argc < 3)
		return 1;
	if(int r = LoadSO(argv[1]); r != 0)
		return 2;
#ifdef __linux__
	// Let the kernel kill us if our parent dies.
	prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif // __linux__
#ifndef _WIN32
	// Close standard pipes (they are unused).
	if(fclose(stdin) != 0)
		return 3;
	if(fclose(stdout) != 0)
		return 4;
	if(fclose(stderr) != 0)
		return 5;
	// Catch interrupt signal and do nothing with it (helps with terminals).
	if(signal(SIGINT, [](int /*unused*/){}) == SIG_ERR)
		return 6;
#endif // _WIN32
	try
	{
		ipc::shared_memory_object shm(ipc::open_only, argv[2], ipc::read_write);
		ipc::mapped_region r(shm, ipc::read_write);
		hss = static_cast<Ignis::Hornet::SharedSegment*>(r.get_address());
		MainLoop();
	}
	catch(const ipc::interprocess_exception& e)
	{
		DLOpen::UnloadObject(handle);
		return 7;
	}
	DLOpen::UnloadObject(handle);
	return 0;
}
