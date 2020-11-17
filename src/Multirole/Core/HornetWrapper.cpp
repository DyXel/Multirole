#include "HornetWrapper.hpp"

#include "IDataSupplier.hpp"
#include "IScriptSupplier.hpp"
#include "ILogger.hpp"
#include "../../HornetCommon.hpp"
#include "../../Process.hpp"

namespace Ignis::Multirole::Core
{

#include "../../Read.inl"
#include "../../Write.inl"

inline std::string MakeHornetName(uintptr_t addr)
{
	std::array<char, 25U> buf;
	std::snprintf(buf.data(), buf.size(), "Hornet0x%lX", addr);
	// Make sure the shared memory object doesn't exist before attempting
	// to create it again.
	ipc::shared_memory_object::remove(buf.data());
	return std::string(buf.data());
}

inline ipc::shared_memory_object MakeShm(const std::string& str)
{
	ipc::shared_memory_object shm(ipc::create_only, str.data(), ipc::read_write);
	shm.truncate(sizeof(Hornet::SharedSegment));
	return shm;
}

// public

HornetWrapper::HornetWrapper(std::string_view absFilePath) :
	shmName(MakeHornetName(reinterpret_cast<uintptr_t>(this))),
	shm(MakeShm(shmName)),
	region(shm, ipc::read_write),
	hss(nullptr)
{
	void* addr = region.get_address();
	hss = new (addr) Hornet::SharedSegment();
	Process::Launch("./hornet", absFilePath.data(), shmName.data());
	// TODO: check if hornet is alive and ready
}

HornetWrapper::~HornetWrapper()
{
	{
		auto lock = NotifyAndWait(Hornet::Action::EXIT);
	} // NOTE: needed to make sure lock doesnt exist when segment dtor is called
	// TODO: wait for hornet to quit
	hss->~SharedSegment();
	ipc::shared_memory_object::remove(shmName.data());
}

std::pair<int, int> HornetWrapper::Version()
{
	auto lock = NotifyAndWait(Hornet::Action::OCG_GET_VERSION);
	const auto* rptr = hss->bytes.data();
	return
	{
		Read<int>(rptr),
		Read<int>(rptr),
	};
}

IWrapper::Duel HornetWrapper::CreateDuel(const DuelOptions& opts)
{
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
	auto lock = NotifyAndWait(Hornet::Action::OCG_CREATE_DUEL);
	const auto* rptr = hss->bytes.data();
	if(Read<int>(rptr) != OCG_DUEL_CREATION_SUCCESS)
		return nullptr; // TODO: throw?
	return Read<OCG_Duel>(rptr);
}

void HornetWrapper::DestroyDuel(Duel duel)
{
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	auto lock = NotifyAndWait(Hornet::Action::OCG_DESTROY_DUEL);
}

void HornetWrapper::AddCard(Duel duel, const OCG_NewCardInfo& info)
{
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	Write<OCG_NewCardInfo>(wptr, info);
	auto lock = NotifyAndWait(Hornet::Action::OCG_DUEL_NEW_CARD);
}

void HornetWrapper::Start(Duel duel)
{
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	auto lock = NotifyAndWait(Hornet::Action::OCG_START_DUEL);
}

IWrapper::DuelStatus HornetWrapper::Process(Duel duel)
{
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	auto lock = NotifyAndWait(Hornet::Action::OCG_DUEL_PROCESS);
	const auto* rptr = hss->bytes.data();
	return DuelStatus{Read<int>(rptr)};
}

IWrapper::Buffer HornetWrapper::GetMessages(Duel duel)
{
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	auto lock = NotifyAndWait(Hornet::Action::OCG_DUEL_GET_MESSAGE);
	const auto* rptr = hss->bytes.data();
	const auto size = static_cast<std::size_t>(Read<uint32_t>(rptr));
	Buffer buffer(size);
	std::memcpy(buffer.data(), rptr, size);
	return buffer;
}

void HornetWrapper::SetResponse(Duel duel, const Buffer& buffer)
{
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	Write<std::size_t>(wptr, buffer.size());
	std::memcpy(wptr, buffer.data(), buffer.size());
	auto lock = NotifyAndWait(Hornet::Action::OCG_DUEL_SET_RESPONSE);
}

int HornetWrapper::LoadScript(Duel duel, std::string_view name, std::string_view str)
{
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	Write<std::size_t>(wptr, name.size());
	std::memcpy(wptr, name.data(), name.size());
	wptr += name.size();
	Write<std::size_t>(wptr, str.size());
	std::memcpy(wptr, str.data(), str.size());
	auto lock = NotifyAndWait(Hornet::Action::OCG_LOAD_SCRIPT);
	const auto* rptr = hss->bytes.data();
	return Read<int>(rptr);
}

std::size_t HornetWrapper::QueryCount(Duel duel, uint8_t team, uint32_t loc)
{
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	Write<uint8_t>(wptr, team);
	Write<uint32_t>(wptr, loc);
	auto lock = NotifyAndWait(Hornet::Action::OCG_DUEL_QUERY_COUNT);
	const auto* rptr = hss->bytes.data();
	return static_cast<std::size_t>(Read<uint32_t>(rptr));
}

IWrapper::Buffer HornetWrapper::Query(Duel duel, const QueryInfo& info)
{
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	Write<OCG_QueryInfo>(wptr, info);
	auto lock = NotifyAndWait(Hornet::Action::OCG_DUEL_QUERY);
	const auto* rptr = hss->bytes.data();
	const auto size = static_cast<std::size_t>(Read<uint32_t>(rptr));
	Buffer buffer(size);
	std::memcpy(buffer.data(), rptr, size);
	return buffer;
}

IWrapper::Buffer HornetWrapper::QueryLocation(Duel duel, const QueryInfo& info)
{
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	Write<OCG_QueryInfo>(wptr, info);
	auto lock = NotifyAndWait(Hornet::Action::OCG_DUEL_QUERY_LOCATION);
	const auto* rptr = hss->bytes.data();
	const auto size = static_cast<std::size_t>(Read<uint32_t>(rptr));
	Buffer buffer(size);
	std::memcpy(buffer.data(), rptr, size);
	return buffer;
}

IWrapper::Buffer HornetWrapper::QueryField(Duel duel)
{
	auto* wptr = hss->bytes.data();
	Write<OCG_Duel>(wptr, duel);
	auto lock = NotifyAndWait(Hornet::Action::OCG_DUEL_QUERY_FIELD);
	const auto* rptr = hss->bytes.data();
	auto size = static_cast<std::size_t>(Read<uint32_t>(rptr));
	Buffer buffer(size);
	std::memcpy(buffer.data(), rptr, size);
	return buffer;
}

Hornet::LockType HornetWrapper::NotifyAndWait(Hornet::Action act)
{
	Hornet::LockType lock(hss->mtx);
	hss->act = act;
	hss->cv.notify_one();
	hss->cv.wait(lock, [&](){return hss->act != act;});
	if(hss->act == Hornet::Action::NO_WORK)
		return lock;
	lock.unlock();
	switch(hss->act)
	{
		case Hornet::Action::CB_DATA_READER:
		{
			const auto* rptr = hss->bytes.data();
			auto* supplier = static_cast<IDataSupplier*>(Read<void*>(rptr));
			const OCG_CardData data = supplier->DataFromCode(Read<uint32_t>(rptr));
			auto* wptr = hss->bytes.data();
			Write<OCG_CardData>(wptr, data);
			for(uint16_t* wptr2 = data.setcodes; *wptr2 != 0U; wptr2++)
				Write<uint16_t>(wptr, *wptr2);
			Write<uint16_t>(wptr, 0U);
			auto lock2 = NotifyAndWait(Hornet::Action::CB_DONE);
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
			auto lock2 = NotifyAndWait(Hornet::Action::CB_DONE);
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
			auto lock2 = NotifyAndWait(Hornet::Action::CB_DONE);
			break;
		}
		case Hornet::Action::CB_DATA_READER_DONE:
		{
			const auto* rptr = hss->bytes.data();
			auto* supplier = static_cast<IDataSupplier*>(Read<void*>(rptr));
			const auto data = Read<OCG_CardData>(rptr);
			supplier->DataUsageDone(data);
			auto lock2 = NotifyAndWait(Hornet::Action::CB_DONE);
			break;
		}
		default: // TODO: throw UB exception
			break;
	}
	lock.lock();
	return lock;
}

} // namespace Ignis::Multirole::Core
