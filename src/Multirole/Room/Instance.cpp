#include "Instance.hpp"

#include "../IRoomManager.hpp"

namespace Ignis::Multirole::Room
{

Instance::Instance(CreateInfo&& info)
	:
	owner(info.owner),
	strand(info.ioCtx),
	tagg(*this),
	ctx({
		tagg,
		info.coreProvider,
		info.replayManager,
		info.scriptProvider,
		std::move(info.cdb),
		std::move(info.hostInfo),
		std::move(info.limits),
		std::move(info.banlist)}),
	state(State::Waiting{nullptr}),
	name(std::move(info.name)),
	notes(std::move(info.notes)),
	pass(std::move(info.pass)),
	id(0U)
{}

void Instance::RegisterToOwner()
{
	uint32_t seed;
	std::tie(id, seed) = owner.Add(shared_from_this());
	ctx.SetId(id);
	ctx.SetRngSeed(seed);
}

bool Instance::Started() const
{
	return !std::holds_alternative<State::Waiting>(state);
}

bool Instance::CheckPassword(std::string_view str) const
{
	return pass.empty() || pass == str;
}

Instance::Properties Instance::GetProperties() const
{
	return Properties
	{
		ctx.HostInfo(),
		notes,
		!pass.empty(),
		Started(),
		id,
		ctx.GetDuelistsNames()
	};
}

void Instance::TryClose()
{
	auto self(shared_from_this());
	asio::post(strand,
	[this, self]()
	{
		Dispatch(Event::Close{});
	});
}

void Instance::Add(std::shared_ptr<Client> client)
{
	std::scoped_lock lock(mClients);
	clients.insert(client);
}

void Instance::Remove(std::shared_ptr<Client> client)
{
	std::scoped_lock lock(mClients);
	clients.erase(client);
	if(clients.empty())
		owner.Remove(id);
}

asio::io_context::strand& Instance::Strand()
{
	return strand;
}

void Instance::Dispatch(const EventVariant& e)
{
	for(StateOpt newState = std::visit(ctx, state, e); newState;)
	{
		state = std::move(*newState);
		newState = std::visit(ctx, state);
	}
}

} // namespace Ignis::Multirole::Room
