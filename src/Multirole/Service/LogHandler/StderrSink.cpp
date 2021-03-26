#include "StderrSink.hpp"

#include <iostream>

#include "StreamFormat.hpp"

namespace Ignis::Multirole::LogHandlerDetail
{

StderrSink::StderrSink(std::mutex& mtx) :
	mtx(mtx)
{}

StderrSink::~StderrSink() noexcept = default;

void StderrSink::Log(const Timestamp& ts, const SinkLogProps& props, std::string_view str) noexcept
{
	std::scoped_lock lock(mtx);
	StreamFormat(std::cerr, ts, props, str);
}

} // namespace Ignis::Multirole::LogHandlerDetail
