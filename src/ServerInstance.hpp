#ifndef SERVERINSTANCE_HPP
#define SERVERINSTANCE_HPP
#include <memory>
#include <asio.hpp>

namespace Placeholder4
{

class ServerInstance final
{
public:
	ServerInstance();
	int Run();
private:
	asio::io_context ioContext;
	asio::signal_set signalSet;

	void Terminate();
};

} // namespace Placeholder4

#endif // SERVERINSTANCE_HPP
