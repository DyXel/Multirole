#ifndef SERVERINSTANCE_HPP
#define SERVERINSTANCE_HPP
#include <memory>
#include "ICoreAPI.hpp"

namespace Placeholder4
{

class ServerInstance final
{
public:
	ServerInstance();
	int Run();
private:
	std::shared_ptr<ICoreAPI> core;
};

} // namespace Placeholder4

#endif // SERVERINSTANCE_HPP
