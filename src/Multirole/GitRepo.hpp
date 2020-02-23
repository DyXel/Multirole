#ifndef GITREPO_HPP
#define GITREPO_HPP
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
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
	using Credentials = std::pair<std::string, std::string>;

	GitRepo(asio::io_context& ioCtx, IAsyncLogger& l, const nlohmann::json& opts);
	~GitRepo();

	// Remove copy operations and move assignment
	GitRepo(const GitRepo&) = delete;
	GitRepo& operator=(const GitRepo&) = delete;
	GitRepo& operator=(GitRepo&&) = delete;

	// Default move constructor
	GitRepo(GitRepo&& other) = default;

	void AddObserver(IGitRepoObserver& obs);
private:
	IAsyncLogger& logger;
	const std::string token;
	const std::string remote;
	const std::string path;
	std::unique_ptr<Credentials> credPtr;
	git_repository* repo;
	std::vector<IGitRepoObserver*> observers;

	void Callback(std::string_view payload) override;

	bool CheckIfRepoExists() const;
	void Clone();
	void Fetch();
	std::vector<std::string> GetFilesDiff() const;
	void ResetToFetchHead();
};

} // namespace Multirole

} // namespace Ignis

#endif // GITREPO_HPP
