#include "HornetWrapper.hpp"

#include <cinttypes> // PRIXPTR
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "IDataSupplier.hpp"
#include "IScriptSupplier.hpp"
#include "ILogger.hpp"
#include "../I18N.hpp"
#include "../../HornetCommon.hpp"
#define PROCESS_IMPLEMENTATION
#include "../../Process.hpp"

#ifndef MULTIROLE_HORNET_MAX_LOOP_COUNT
#define MULTIROLE_HORNET_MAX_LOOP_COUNT 256U
#endif // MULTIROLE_HORNET_MAX_LOOP_COUNT

#ifndef MULTIROLE_HORNET_MAX_WAIT_COUNT
#define MULTIROLE_HORNET_MAX_WAIT_COUNT 15U
#endif // MULTIROLE_HORNET_MAX_WAIT_COUNT

namespace Ignis::Multirole::Core
{

namespace
{

static_assert(MULTIROLE_HORNET_MAX_LOOP_COUNT >= 1U);
static_assert(MULTIROLE_HORNET_MAX_WAIT_COUNT >= 1U);

// Time in seconds to wait before concluding that Hornet is unresponsive.
const auto SECS_TO_KILL = boost::posix_time::seconds(1U);
// Time in seconds per wait round to check if Hornet is dead. This multiplied by
// MULTIROLE_HORNET_MAX_WAIT_COUNT is the total amount of time to spend before
// giving up and throwing a exception.
const auto SECS_PER_WAIT = boost::posix_time::seconds(2U);

#include "../../Read.inl"
#include "../../Write.inl"

inline std::string MakeHornetName(uintptr_t addr)
{
#define PREFIX "Hornet0x%"
	constexpr auto MAX_DIGIT_CNT = std::numeric_limits<uintptr_t>::digits / 4;
	std::array<char, sizeof(PREFIX) + MAX_DIGIT_CNT> buf{};
	int sz = std::snprintf(buf.data(), buf.size(), PREFIX PRIXPTR, addr);
#undef PREFIX
	return std::string(buf.data(), static_cast<std::size_t>(sz));
}

inline ipc::shared_memory_object MakeShm(const std::string& str)
{
	// Make sure the shared memory object doesn't exist before attempting
	// to create it again.
	ipc::shared_memory_object::remove(str.data());
	ipc::shared_memory_object shm(ipc::create_only, str.data(), ipc::read_write);
	shm.truncate(sizeof(Hornet::SharedSegment));
	return shm;
}

} // namespace

// public

HornetWrapper::HornetWrapper(std::string_view absFilePath) :
	shmName(MakeHornetName(reinterpret_cast<uintptr_t>(this))),
	shm(MakeShm(shmName)),
	region(shm, ipc::read_write),
	hss(nullptr)
{
	void* addr = region.get_address();
	hss = new (addr) Hornet::SharedSegment();
	const auto p = Process::Launch("./hornet", absFilePath.data(), shmName.data());
	if(!p.second)
	{
		DestroySharedSegment();
		throw std::runtime_error(I18N::HWRAPPER_UNABLE_TO_LAUNCH);
	}
	proc = p.first;
	try
	{
		NotifyAndWait(Hornet::Action::HEARTBEAT);
	}
	catch(Core::Exception& e)
	{
		// NOTE: Not check necessary as HEARTBEAT is under our control.
		Process::CleanUp(proc);
		DestroySharedSegment();
		throw std::runtime_error(I18N::HWRAPPER_HEARTBEAT_FAILURE);
	}
}

HornetWrapper::~HornetWrapper()
{
	// Scenarios:
	// 1. Hornet waits on CV as normal.
	// 2. Hornet is dead.
	// 3. Hornet is unresponsive.
	//  a. It can become responsive at any point.
	//  b. It could perform callback operation at any point.
	try
	{
		constexpr auto EXIT = Hornet::Action::EXIT;
		// Attempt to notify as intended.
		Hornet::LockType lock(hss->mtx);
		hss->act = EXIT;
		hss->cv.notify_one();
		// We do a small timed wait to verify if Hornet is unresponsive.
		if(!hss->cv.wait_for(lock, SECS_TO_KILL, [&](){return hss->act != EXIT;}))
		{
			// Hornet was unresponsive or dead. Lets guarantee that it is dead.
			Process::Kill(proc);
		}
		else if(hss->act == Hornet::Action::EXIT_CONFIRMED)
		{
			// At this point, termination is imminent or already happened.
		}
		else if(hss->act != EXIT)
		{
			// It got responsive and tried to signal us *just* as we signal it
			// to quit. In that case we signal it again with EXIT, except this
			// time we never wait as it should terminate by itself in both cases
			// where it signaled us with callback data or with NO_WORK.
			hss->act = EXIT;
			hss->cv.notify_one();
		}
	}
	catch(const ipc::interprocess_exception& e)
	{
		// The only time the code above would throw an exception is when we
		// somehow tried to lock the mutex while Hornet held it and died while
		// doing so; This might happen when we thought it was unresponsive,
		// killed it, but then it took the lock before it was killed, very
		// unlikely, but due to non-deterministic scheduling, can happen.
	}
	// Let's wait until Hornet has terminated, the try block above should
	// guarantee that it has already happened or will happen imminently.
	while(Process::IsRunning(proc));
	Process::CleanUp(proc);
	DestroySharedSegment();
}

std::pair<int, int> HornetWrapper::Version()
{
	std::scoped_lock lock(mtx);
	NotifyAndWait(Hornet::Action::OCG_GET_VERSION);
	const auto* rptr = hss->bytes.data();
	return
	{
		Read<int>(rptr),
		Read<int>(rptr),
	};
}

IWrapper::Duel HornetWrapper::CreateDuel(const DuelOptions& opts)
{
	std::scoped_lock lock(mtx);
	auto* wptr = hss->bytes.data();
	Write<OCG_DuelOptions>(wptr,
	{
		{opts.seed[0U], opts.seed[1U], opts.seed[2U], opts.seed[3U]},
		opts.flags,
		opts.team1,
		opts.team2,
		nullptr, // NOTE: Set on Hornet
		&opts.dataSupplier,
		nullptr, // NOTE: Set on Hornet
		&opts.scriptSupplier,
		nullptr, // NOTE: Set on Hornet
		opts.optLogger,
		nullptr, // NOTE: Set on Hornet
		&opts.dataSupplier,
		0
	});
	NotifyAndWait(Hornet::Action::OCG_CREATE_DUEL);
	const auto* rptr = hss->bytes.data();
	if(Read<int>(rptr) != OCG_DUEL_CREATION_SUCCESS)
		throw Core::Exception(I18N::HWRAPPER_EXCEPT_CREATE_DUEL);
	return Read<OCG_Duel>(rptr);
}

void HornetWrapper::DestroyDuel(Duel duel)
{
	std::scoped_lock lock(mtx);
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	NotifyAndWait(Hornet::Action::OCG_DESTROY_DUEL);
}

void HornetWrapper::AddCard(Duel duel, const OCG_NewCardInfo& info)
{
	std::scoped_lock lock(mtx);
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	Write<OCG_NewCardInfo>(wptr, info);
	NotifyAndWait(Hornet::Action::OCG_DUEL_NEW_CARD);
}

void HornetWrapper::Start(Duel duel)
{
	std::scoped_lock lock(mtx);
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	NotifyAndWait(Hornet::Action::OCG_START_DUEL);
}

IWrapper::DuelStatus HornetWrapper::Process(Duel duel)
{
	std::scoped_lock lock(mtx);
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	NotifyAndWait(Hornet::Action::OCG_DUEL_PROCESS);
	const auto* rptr = hss->bytes.data();
	return DuelStatus{Read<int>(rptr)};
}

IWrapper::Buffer HornetWrapper::GetMessages(Duel duel)
{
	std::scoped_lock lock(mtx);
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	NotifyAndWait(Hornet::Action::OCG_DUEL_GET_MESSAGE);
	const auto* rptr = hss->bytes.data();
	const auto size = static_cast<std::size_t>(Read<uint32_t>(rptr));
	Buffer buffer(size);
	std::memcpy(buffer.data(), rptr, size);
	return buffer;
}

void HornetWrapper::SetResponse(Duel duel, const Buffer& buffer)
{
	std::scoped_lock lock(mtx);
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	Write<std::size_t>(wptr, buffer.size());
	std::memcpy(wptr, buffer.data(), buffer.size());
	NotifyAndWait(Hornet::Action::OCG_DUEL_SET_RESPONSE);
}

int HornetWrapper::LoadScript(Duel duel, std::string_view name, std::string_view str)
{
	std::scoped_lock lock(mtx);
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	Write<std::size_t>(wptr, name.size() + 1U);
	std::memcpy(wptr, name.data(), name.size());
	wptr += name.size();
	Write<uint8_t>(wptr, 0);
	Write<std::size_t>(wptr, str.size());
	std::memcpy(wptr, str.data(), str.size());
	NotifyAndWait(Hornet::Action::OCG_LOAD_SCRIPT);
	const auto* rptr = hss->bytes.data();
	return Read<int>(rptr);
}

std::size_t HornetWrapper::QueryCount(Duel duel, uint8_t team, uint32_t loc)
{
	std::scoped_lock lock(mtx);
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	Write<uint8_t>(wptr, team);
	Write<uint32_t>(wptr, loc);
	NotifyAndWait(Hornet::Action::OCG_DUEL_QUERY_COUNT);
	const auto* rptr = hss->bytes.data();
	return static_cast<std::size_t>(Read<uint32_t>(rptr));
}

IWrapper::Buffer HornetWrapper::Query(Duel duel, const QueryInfo& info)
{
	std::scoped_lock lock(mtx);
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	Write<OCG_QueryInfo>(wptr, info);
	NotifyAndWait(Hornet::Action::OCG_DUEL_QUERY);
	const auto* rptr = hss->bytes.data();
	const auto size = static_cast<std::size_t>(Read<uint32_t>(rptr));
	Buffer buffer(size);
	std::memcpy(buffer.data(), rptr, size);
	return buffer;
}

IWrapper::Buffer HornetWrapper::QueryLocation(Duel duel, const QueryInfo& info)
{
	std::scoped_lock lock(mtx);
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	Write<OCG_QueryInfo>(wptr, info);
	NotifyAndWait(Hornet::Action::OCG_DUEL_QUERY_LOCATION);
	const auto* rptr = hss->bytes.data();
	const auto size = static_cast<std::size_t>(Read<uint32_t>(rptr));
	Buffer buffer(size);
	std::memcpy(buffer.data(), rptr, size);
	return buffer;
}

IWrapper::Buffer HornetWrapper::QueryField(Duel duel)
{
	std::scoped_lock lock(mtx);
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	NotifyAndWait(Hornet::Action::OCG_DUEL_QUERY_FIELD);
	const auto* rptr = hss->bytes.data();
	auto size = static_cast<std::size_t>(Read<uint32_t>(rptr));
	Buffer buffer(size);
	std::memcpy(buffer.data(), rptr, size);
	return buffer;
}

void HornetWrapper::DestroySharedSegment()
{
	// NOTE: From Boost.Interprocess documentation:
	// Unlike std::condition_variable in C++11, it is NOT safe to invoke the
	// destructor if all threads have been only notified. It is required that
	// they have exited their respective wait functions.
	// If this is called while Hornet is waiting on the condition variable
	// the calling thread will hang, or worse, the whole process will crash.
	hss->~SharedSegment();
	ipc::shared_memory_object::remove(shmName.data());
}

void HornetWrapper::NotifyAndWait(Hornet::Action act)
{
	Hornet::Action recvAct = Hornet::Action::NO_WORK;
	std::size_t loopCount = 0U;
	do
	{
		if(loopCount++ == MULTIROLE_HORNET_MAX_LOOP_COUNT)
			throw Core::Exception(I18N::HWRAPPER_EXCEPT_MAX_LOOP_COUNT);
		// Atomically perform action and fetch next one, if any.
		{
			std::size_t waitCount = 0U;
			Hornet::LockType lock(hss->mtx);
			hss->act = act;
			hss->cv.notify_one();
			while(!hss->cv.wait_for(lock, SECS_PER_WAIT, [&](){return hss->act != act;}))
			{
				if(!Process::IsRunning(proc))
					throw Core::Exception(I18N::HWRAPPER_EXCEPT_PROC_CRASHED);
				if(++waitCount >= MULTIROLE_HORNET_MAX_WAIT_COUNT)
					throw Core::Exception(I18N::HWRAPPER_EXCEPT_PROC_UNRESPONSIVE);
			}
			recvAct = hss->act;
		}
		// If action sent by hornet requires handling then it should be
		// implemented here and hornet should always be notified back,
		// otherwise it'll wait endlessly.
		switch(recvAct)
		{
		case Hornet::Action::CB_DATA_READER:
		{
			const auto* rptr = hss->bytes.data();
			auto* supplier = static_cast<IDataSupplier*>(Read<void*>(rptr));
			const OCG_CardData data = supplier->DataFromCode(Read<uint32_t>(rptr));
			auto* wptr = hss->bytes.data();
			Write<OCG_CardData>(wptr, data);
			if(data.setcodes != nullptr)
				for(uint16_t* wptr2 = data.setcodes; *wptr2 != 0U; wptr2++)
					Write<uint16_t>(wptr, *wptr2);
			Write<uint16_t>(wptr, 0U);
			act = Hornet::Action::CB_DONE;
			break;
		}
		case Hornet::Action::CB_SCRIPT_READER:
		{
			const auto* rptr = hss->bytes.data();
			auto* supplier = static_cast<IScriptSupplier*>(Read<void*>(rptr));
			const auto nameSz = Read<std::size_t>(rptr);
			const std::string_view nameSv(reinterpret_cast<const char*>(rptr), nameSz);
			const auto script = supplier->ScriptFromFilePath(nameSv);
			const auto size = IScriptSupplier::GetSize(script);
			auto* wptr = hss->bytes.data();
			Write<std::size_t>(wptr, size);
			if(const char* const data = IScriptSupplier::GetData(script); data != nullptr)
				std::memcpy(wptr, data, size);
			act = Hornet::Action::CB_DONE;
			break;
		}
		case Hornet::Action::CB_LOG_HANDLER:
		{
			const auto* rptr = hss->bytes.data();
			auto* logger = static_cast<ILogger*>(Read<void*>(rptr));
			const auto type = ILogger::LogType{Read<int>(rptr)};
			const auto strSz = Read<std::size_t>(rptr);
			const std::string_view strSv(reinterpret_cast<const char*>(rptr), strSz);
			if(logger != nullptr)
				logger->Log(type, strSv);
			act = Hornet::Action::CB_DONE;
			break;
		}
		case Hornet::Action::CB_DATA_READER_DONE:
		{
			const auto* rptr = hss->bytes.data();
			auto* supplier = static_cast<IDataSupplier*>(Read<void*>(rptr));
			const auto data = Read<OCG_CardData>(rptr);
			supplier->DataUsageDone(data);
			act = Hornet::Action::CB_DONE;
			break;
		}
		// Explicitly ignore these, in case we ever add more functionality...
		case Hornet::Action::NO_WORK:
		case Hornet::Action::HEARTBEAT:
		case Hornet::Action::EXIT:
		case Hornet::Action::EXIT_CONFIRMED:
		case Hornet::Action::OCG_GET_VERSION:
		case Hornet::Action::OCG_CREATE_DUEL:
		case Hornet::Action::OCG_DESTROY_DUEL:
		case Hornet::Action::OCG_DUEL_NEW_CARD:
		case Hornet::Action::OCG_START_DUEL:
		case Hornet::Action::OCG_DUEL_PROCESS:
		case Hornet::Action::OCG_DUEL_GET_MESSAGE:
		case Hornet::Action::OCG_DUEL_SET_RESPONSE:
		case Hornet::Action::OCG_LOAD_SCRIPT:
		case Hornet::Action::OCG_DUEL_QUERY_COUNT:
		case Hornet::Action::OCG_DUEL_QUERY:
		case Hornet::Action::OCG_DUEL_QUERY_LOCATION:
		case Hornet::Action::OCG_DUEL_QUERY_FIELD:
		case Hornet::Action::CB_DONE:
			break;
		}
	}while(recvAct != Hornet::Action::NO_WORK);
}

} // namespace Ignis::Multirole::Core
