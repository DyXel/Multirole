#ifndef GITREPO_HPP
#define GITREPO_HPP
#include <string>
#include <vector>

#include <boost/json/fwd.hpp>

#include "IGitRepoObserver.hpp"
#include "Endpoint/Webhook.hpp"

struct git_repository;

namespace Ignis::Multirole
{

class GitRepo final : public Endpoint::Webhook
{
public:
	using Credentials = std::pair<std::string, std::string>;

	GitRepo(boost::asio::io_context& ioCtx, const boost::json::value& opts);
	~GitRepo();

	// Remove copy and move operations.
	GitRepo(const GitRepo&) = delete;
	GitRepo(GitRepo&&) = delete;
	GitRepo& operator=(const GitRepo&) = delete;
	GitRepo& operator=(GitRepo&&) = delete;

	void AddObserver(IGitRepoObserver& obs);
private:
	const std::string token;
	const std::string remote;
	const std::string path;
	std::unique_ptr<Credentials> credPtr;
	git_repository* repo;
	std::vector<IGitRepoObserver*> observers;

	// Endpoint::Webhook override
	void Callback(std::string_view payload) override;

	bool CheckIfRepoExists() const;
	void Clone();
	void Fetch();
	void ResetToFetchHead();

	GitDiff GetFilesDiff() const;
	std::vector<std::string> GetTrackedFiles() const;
};

} // namespace Ignis::Multirole

#endif // GITREPO_HPP
