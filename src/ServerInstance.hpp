#ifndef SERVERINSTANCE_HPP
#define SERVERINSTANCE_HPP
#include <memory>
#include <asio.hpp>

namespace Ignis
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

} // namespace Ignis

#endif // SERVERINSTANCE_HPP
