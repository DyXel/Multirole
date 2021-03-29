#ifndef MULTIROLE_SERVICE_LOGHANDLER_FILESINK_HPP
#define MULTIROLE_SERVICE_LOGHANDLER_FILESINK_HPP
#include "ISink.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

namespace Ignis::Multirole::LogHandlerDetail
{

class FileSink final : public ISink
{
public:
	FileSink(const boost::filesystem::path& p);
	~FileSink() noexcept;
	void Log(const Timestamp& ts, const SinkLogProps& props, std::string_view str) noexcept override;
private:
	std::ofstream f;
};

} // namespace Ignis::Multirole::LogHandlerDetail

#endif // MULTIROLE_SERVICE_LOGHANDLER_FILESINK_HPP
