#include "Instance.hpp"

namespace Ignis::Multirole::Room
{

Instance::Instance(CreateInfo& info)
	:
	strand(info.ioCtx),
	tagg(*this),
	notes(std::move(info.notes)),
	pass(std::move(info.pass)),
	isPrivate(!pass.empty()),
	ctx({info.svc, tagg, info.id, info.seed, std::move(info.banlist), info.hostInfo, info.limits}),
	state(State::Waiting{nullptr})
{}

bool Instance::IsPrivate() const
{
	return isPrivate;
}

bool Instance::Started() const
{
	return !std::holds_alternative<State::Waiting>(state);
}

const std::string& Instance::Notes() const
{
	return notes;
}

const YGOPro::HostInfo& Instance::HostInfo() const
{
	return ctx.HostInfo();
}

std::map<uint8_t, std::string> Instance::DuelistNames() const
{
	return ctx.GetDuelistsNames();
}

bool Instance::CheckPassword(std::string_view str) const
{
	return !isPrivate || pass == str;
}

bool Instance::CheckKicked(const boost::asio::ip::address& addr) const
{
	std::scoped_lock lock(mKicked);
	return kicked.count(addr) > 0U;
}

void Instance::TryClose()
{
	auto self(shared_from_this());
	boost::asio::post(strand,
	[this, self]()
	{
		Dispatch(Event::Close{});
	});
}

void Instance::AddKicked(const boost::asio::ip::address& addr)
{
	std::scoped_lock lock(mKicked);
	kicked.insert(addr);
}

void Instance::Add(const std::shared_ptr<Client>& client)
{
	std::scoped_lock lock(mClients);
	clients.insert(client);
}

void Instance::Remove(const std::shared_ptr<Client>& client)
{
	std::scoped_lock lock(mClients);
	clients.erase(client);
}

boost::asio::io_context::strand& Instance::Strand()
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
