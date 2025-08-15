#ifndef MULTIROLE_SERVICE_LOGHANDLER_FILESINK_HPP
#define MULTIROLE_SERVICE_LOGHANDLER_FILESINK_HPP
#include "ISink.hpp"

#include <filesystem>
#include <fstream>

namespace Ignis::Multirole::LogHandlerDetail
{

class FileSink final : public ISink
{
public:
	FileSink(const std::filesystem::path& p);
	~FileSink() noexcept;
	void Log(const Timestamp& ts, const SinkLogProps& props, std::string_view str) noexcept override;
private:
	std::ofstream f;
};

} // namespace Ignis::Multirole::LogHandlerDetail

#endif // MULTIROLE_SERVICE_LOGHANDLER_FILESINK_HPP
