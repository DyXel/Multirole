#ifndef SERVERINSTANCE_HPP
#define SERVERINSTANCE_HPP
#include <memory>
#include <asio.hpp>
#include <nlohmann/json.hpp>


namespace Ignis
{

class LobbyListEndpoint;

class ServerInstance final
{
public:
	ServerInstance();
	int Run();
private:
	asio::io_context ioContext;
	nlohmann::json cfg;
	asio::signal_set signalSet;
	std::shared_ptr<LobbyListEndpoint> lle;

	void Stop();
	void Terminate();
};

} // namespace Ignis

#endif // SERVERINSTANCE_HPP
