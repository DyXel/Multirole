#include "Instance.hpp"

namespace Ignis::Multirole::Room
{

Instance::Instance(CreateInfo& info)
	:
	strand(info.ioCtx),
	tagg(*this),
	notes(std::move(info.notes)),
	pass(std::move(info.pass)),
	ctx({
		info.svc,
		tagg,
		info.id,
		info.seed,
		std::move(info.banlist),
		info.hostInfo,
		info.limits,
		!pass.empty(),
		notes}),
	state(State::Waiting{nullptr})
{}

bool Instance::IsPrivate() const
{
	return ctx.IsPrivate();
}

bool Instance::Started() const
{
	std::shared_lock lock(mState);
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
	return !IsPrivate() || pass == str;
}

bool Instance::CheckKicked(std::string_view ip) const
{
	std::scoped_lock lock(mKicked);
	return kicked.count(ip.data()) > 0U;
}

bool Instance::TryClose()
{
	if(Started())
		return false;
	auto self(shared_from_this());
	boost::asio::post(strand,
	[this, self]()
	{
		Dispatch(Event::Close{});
	});
	return true;
}

void Instance::AddKicked(std::string_view ip)
{
	std::scoped_lock lock(mKicked);
	kicked.insert(ip.data());
}

boost::asio::io_context::strand& Instance::Strand()
{
	return strand;
}

void Instance::Dispatch(const EventVariant& e)
{
	std::scoped_lock lock(mState);
	for(StateOpt newState = std::visit(ctx, state, e); newState;)
	{
		state = std::move(*newState);
		newState = std::visit(ctx, state);
	}
}

} // namespace Ignis::Multirole::Room
