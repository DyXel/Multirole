#include "HornetWrapper.hpp"

#include <cinttypes>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "IDataSupplier.hpp"
#include "IScriptSupplier.hpp"
#include "ILogger.hpp"
#include "../I18N.hpp"
#include "../../HornetCommon.hpp"
#define PROCESS_IMPLEMENTATION
#include "../../Process.hpp"

#ifndef MULTIROLE_HORNET_MAX_LOOP_COUNT
#define MULTIROLE_HORNET_MAX_LOOP_COUNT 512U
#endif // MULTIROLE_HORNET_MAX_LOOP_COUNT

#ifndef MULTIROLE_HORNET_MAX_WAIT_COUNT
#define MULTIROLE_HORNET_MAX_WAIT_COUNT 5U
#endif // MULTIROLE_HORNET_MAX_WAIT_COUNT

namespace Ignis::Multirole::Core
{

namespace
{

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
	hss(nullptr),
	hanged(false)
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
		// NOTE: Not necessary to check hanged or kill as HEARTBEAT is
		// under our control.
		Process::CleanUp(proc);
		DestroySharedSegment();
		throw std::runtime_error(I18N::HWRAPPER_HEARTBEAT_FAILURE);
	}
}

HornetWrapper::~HornetWrapper()
{
	// Even if process was hanged, there is no guarantee that it will be now
	// and that hornet is not performing a wait on the condition variable.
	// This avoids deadlocking when calling the shared segment destructor.
	{
		Hornet::LockType lock(hss->mtx);
		hss->act = Hornet::Action::EXIT;
		hss->cv.notify_one();
	}
	// If process is hanged we can't guarantee it'll handle our notification.
	// Kill anyways.
	if(hanged && Process::IsRunning(proc))
		Process::Kill(proc);
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
		opts.seed,
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
		&opts.dataSupplier
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
	Write<std::size_t>(wptr, name.size());
	std::memcpy(wptr, name.data(), name.size());
	wptr += name.size();
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
	// Time to wait before checking for process being dead
	auto NowPlusOffset = []() -> boost::posix_time::ptime
	{
		auto now = boost::posix_time::second_clock::universal_time();
		now += boost::posix_time::seconds(10U);
		return now;
	};
	Hornet::Action recvAct = Hornet::Action::NO_WORK;
	std::size_t loopCount = 0U;
	do
	{
		if(loopCount++ > MULTIROLE_HORNET_MAX_LOOP_COUNT)
		{
			hanged = true;
			throw Core::Exception(I18N::HWRAPPER_EXCEPT_MAX_LOOP_COUNT);
		}
		// Atomically fetch next action, if any.
		{
			std::size_t waitCount = 0U;
			Hornet::LockType lock(hss->mtx);
			hss->act = act;
			hss->cv.notify_one();
			while(!hss->cv.timed_wait(lock, NowPlusOffset(), [&](){return hss->act != act;}))
			{
				if(!Process::IsRunning(proc))
					throw Core::Exception(I18N::HWRAPPER_EXCEPT_PROC_CRASHED);
				if(waitCount++ <= MULTIROLE_HORNET_MAX_WAIT_COUNT)
					continue;
				hanged = true;
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
			std::string script = supplier->ScriptFromFilePath(nameSv);
			auto* wptr = hss->bytes.data();
			Write<std::size_t>(wptr, script.size());
			if(!script.empty())
				std::memcpy(wptr, script.data(), script.size());
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
