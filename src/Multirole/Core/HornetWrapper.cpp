#include "HornetWrapper.hpp"

#include <spdlog/spdlog.h> // TODO: remove

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
	shm.truncate(sizeof(Ignis::Hornet::SharedSegment));
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
	hss = new (addr) Ignis::Hornet::SharedSegment();
	Process::Launch("./hornet", absFilePath.data(), shmName.data());
	spdlog::info("Hornet launched!");
}

HornetWrapper::~HornetWrapper()
{
	// TODO: signal closure and wait for hornet to send goodbye message
	ipc::shared_memory_object::remove(shmName.data());
}

std::pair<int, int> HornetWrapper::Version()
{
	auto lock = NotifyAndWaitCompletion(Hornet::Action::OCG_GET_VERSION);
	const auto* ptr = hss->bytes.data();
	return
	{
		Read<int>(ptr),
		Read<int>(ptr),
	};
}

IWrapper::Duel HornetWrapper::CreateDuel(const DuelOptions& opts)
{
	return OCG_Duel{nullptr};
}

void HornetWrapper::DestroyDuel(Duel duel)
{
	auto* ptr = hss->bytes.data();
	Write<OCG_Duel>(ptr, duel);
	auto lock = NotifyAndWaitCompletion(Hornet::Action::OCG_DESTROY_DUEL);
}

void HornetWrapper::AddCard(Duel duel, const OCG_NewCardInfo& info)
{}

void HornetWrapper::Start(Duel duel)
{
	auto* ptr = hss->bytes.data();
	Write<OCG_Duel>(ptr, duel);
	auto lock = NotifyAndWaitCompletion(Hornet::Action::OCG_START_DUEL);
}

IWrapper::DuelStatus HornetWrapper::Process(Duel duel)
{
	return DuelStatus::DUEL_STATUS_END;
}

IWrapper::Buffer HornetWrapper::GetMessages(Duel duel)
{
	auto* ptr1 = hss->bytes.data();
	Write<OCG_Duel>(ptr1, duel);
	auto lock = NotifyAndWaitCompletion(Hornet::Action::OCG_DUEL_GET_MESSAGE);
	const auto* ptr2 = hss->bytes.data();
	auto size = static_cast<std::size_t>(Read<uint32_t>(ptr2));
	Buffer buffer(size);
	std::memcpy(buffer.data(), ptr2, size);
	return buffer;
}

void HornetWrapper::SetResponse(Duel duel, const Buffer& buffer)
{
	auto* ptr1 = hss->bytes.data();
	Write<OCG_Duel>(ptr1, duel);
	Write<std::size_t>(ptr1, buffer.size());
	std::memcpy(ptr1, buffer.data(), buffer.size());
	auto lock = NotifyAndWaitCompletion(Hornet::Action::OCG_DUEL_SET_RESPONSE);
}

int HornetWrapper::LoadScript(Duel duel, std::string_view name, std::string_view str)
{
	return 1;
}

std::size_t HornetWrapper::QueryCount(Duel duel, uint8_t team, uint32_t loc)
{
	auto* ptr1 = hss->bytes.data();
	Write<OCG_Duel>(ptr1, duel);
	Write<uint8_t>(ptr1, team);
	Write<uint32_t>(ptr1, loc);
	auto lock = NotifyAndWaitCompletion(Hornet::Action::OCG_DUEL_QUERY_COUNT);
	const auto* ptr2 = hss->bytes.data();
	return static_cast<std::size_t>(Read<uint32_t>(ptr2));
}

IWrapper::Buffer HornetWrapper::Query(Duel duel, const QueryInfo& info)
{
	auto* ptr1 = hss->bytes.data();
	Write<OCG_Duel>(ptr1, duel);
	Write<OCG_QueryInfo>(ptr1, info);
	auto lock = NotifyAndWaitCompletion(Hornet::Action::OCG_DUEL_QUERY);
	const auto* ptr2 = hss->bytes.data();
	auto size = static_cast<std::size_t>(Read<uint32_t>(ptr2));
	Buffer buffer(size);
	std::memcpy(buffer.data(), ptr2, size);
	return buffer;
}

IWrapper::Buffer HornetWrapper::QueryLocation(Duel duel, const QueryInfo& info)
{
	auto* ptr1 = hss->bytes.data();
	Write<OCG_Duel>(ptr1, duel);
	Write<OCG_QueryInfo>(ptr1, info);
	auto lock = NotifyAndWaitCompletion(Hornet::Action::OCG_DUEL_QUERY_LOCATION);
	const auto* ptr2 = hss->bytes.data();
	auto size = static_cast<std::size_t>(Read<uint32_t>(ptr2));
	Buffer buffer(size);
	std::memcpy(buffer.data(), ptr2, size);
	return buffer;
}

IWrapper::Buffer HornetWrapper::QueryField(Duel duel)
{
	auto* ptr1 = hss->bytes.data();
	Write<OCG_Duel>(ptr1, duel);
	auto lock = NotifyAndWaitCompletion(Hornet::Action::OCG_DUEL_QUERY_FIELD);
	const auto* ptr2 = hss->bytes.data();
	auto size = static_cast<std::size_t>(Read<uint32_t>(ptr2));
	Buffer buffer(size);
	std::memcpy(buffer.data(), ptr2, size);
	return buffer;
}

HornetWrapper::LockType HornetWrapper::NotifyAndWaitCompletion(Ignis::Hornet::Action act)
{
	LockType lock(hss->mtx);
	hss->cv.notify_one();
	hss->cv.wait(lock, [&](){return hss->act == Hornet::Action::NO_WORK;});
	return lock;
}

} // namespace Ignis::Multirole::Core
