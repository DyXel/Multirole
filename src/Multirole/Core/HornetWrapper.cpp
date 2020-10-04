#include "HornetWrapper.hpp"

#include "IDataSupplier.hpp"
#include "IScriptSupplier.hpp"
#include "ILogger.hpp"

namespace Ignis::Multirole::Core
{

// public

HornetWrapper::HornetWrapper()
{}

HornetWrapper::~HornetWrapper()
{}

IWrapper::Duel HornetWrapper::CreateDuel(const DuelOptions& opts)
{
	return OCG_Duel{nullptr};
}

void HornetWrapper::DestroyDuel(Duel duel)
{}

void HornetWrapper::AddCard(Duel duel, const OCG_NewCardInfo& info)
{}

void HornetWrapper::Start(Duel duel)
{}

IWrapper::DuelStatus HornetWrapper::Process(Duel duel)
{
	return DuelStatus::DUEL_STATUS_END;
}

IWrapper::Buffer HornetWrapper::GetMessages(Duel duel)
{
	return {};
}

void HornetWrapper::SetResponse(Duel duel, const Buffer& buffer)
{}

int HornetWrapper::LoadScript(Duel duel, std::string_view name, std::string_view str)
{
	return 1;
}

std::size_t HornetWrapper::QueryCount(Duel duel, uint8_t team, uint32_t loc)
{
	return 0U;
}

IWrapper::Buffer HornetWrapper::Query(Duel duel, const QueryInfo& info)
{
	return {};
}

IWrapper::Buffer HornetWrapper::QueryLocation(Duel duel, const QueryInfo& info)
{
	return {};
}

IWrapper::Buffer HornetWrapper::QueryField(Duel duel)
{
	return {};
}

} // namespace Ignis::Multirole::Core
