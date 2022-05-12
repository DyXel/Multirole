#include "Instance.hpp"

namespace Ignis::Multirole::Room
{

Instance::Instance(CreateInfo& info) noexcept
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

bool Instance::IsPrivate() const noexcept
{
	return ctx.IsPrivate();
}

bool Instance::Started() const noexcept
{
	return ctx.IsStarted();
}

const std::string& Instance::Notes() const noexcept
{
	return notes;
}

const YGOPro::HostInfo& Instance::HostInfo() const noexcept
{
	return ctx.HostInfo();
}

DuelistsMap Instance::DuelistNames() const noexcept
{
	return ctx.GetDuelistsNames();
}

bool Instance::CheckPassword(std::string_view str) const noexcept
{
	return !IsPrivate() || pass == str;
}

bool Instance::CheckKicked(std::string_view ip) const noexcept
{
	std::scoped_lock lock(mKicked);
	return kicked.count(ip.data()) > 0U;
}

bool Instance::TryClose() noexcept
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

void Instance::AddKicked(std::string_view ip) noexcept
{
	std::scoped_lock lock(mKicked);
	kicked.insert(ip.data());
}

boost::asio::io_context::strand& Instance::Strand() noexcept
{
	return strand;
}

void Instance::Dispatch(const EventVariant& e) noexcept
{
	std::scoped_lock lock(mState);
	for(StateOpt newState = std::visit(ctx, state, e); newState;)
	{
		state = std::move(*newState);
		newState = std::visit(ctx, state);
	}
}

} // namespace Ignis::Multirole::Room
