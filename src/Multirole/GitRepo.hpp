#ifndef GITREPO_HPP
#define GITREPO_HPP
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>
#include "Endpoint/Webhook.hpp"

struct git_repository;

namespace Ignis
{

namespace Multirole
{

class IAsyncLogger;
class IGitRepoObserver;

class GitRepo final : public Endpoint::Webhook
{
public:
	GitRepo(asio::io_context& ioCtx, IAsyncLogger& l, const nlohmann::json& opts);
	~GitRepo();

	void AddObserver(IGitRepoObserver& obs);
private:
	IAsyncLogger& logger;
	const std::string name;
	const std::string token;
	const std::string remote;
	const std::string path;
	git_repository* repo;
	std::vector<IGitRepoObserver*> observers;

	void Callback(std::string_view payload) override;

	bool CheckIfRepoExists() const;
	bool Fetch();
	std::vector<std::string> GetFilesDiff() const;
	void ResetToFetchHead();
};

} // namespace Multirole

} // namespace Ignis

#endif // GITREPO_HPP
