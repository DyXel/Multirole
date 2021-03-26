#include "StdoutSink.hpp"

#include <iostream>

#include "StreamFormat.hpp"

namespace Ignis::Multirole::LogHandlerDetail
{

StdoutSink::StdoutSink(std::mutex& mtx) :
	mtx(mtx)
{}

StdoutSink::~StdoutSink() noexcept = default;

void StdoutSink::Log(const Timestamp& ts, const SinkLogProps& props, std::string_view str) noexcept
{
	std::scoped_lock lock(mtx);
	StreamFormat(std::cout, ts, props, str);
}

} // namespace Ignis::Multirole::LogHandlerDetail
