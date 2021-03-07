#include "GitRepo.hpp"

#include <spdlog/spdlog.h>

#include "libgit2.hpp"

namespace Ignis::Multirole
{

int CredCb(git_cred** out, const char* /*unused*/, const char* /*unused*/, unsigned int aTypes, void* pl)
{
	if((GIT_CREDTYPE_USERPASS_PLAINTEXT & aTypes) == 0U)
		return GIT_EUSER;
	const auto& cred = *static_cast<GitRepo::Credentials*>(pl);
	return git_cred_userpass_plaintext_new(out, cred.first.c_str(), cred.second.c_str());
}

std::string NormalizeDirPath(std::string_view str)
{
	std::string tmp(str);
	if(tmp.back() != '/' || tmp.back() != '\\')
		tmp.push_back('/');
	return tmp;
}

// public

GitRepo::GitRepo(boost::asio::io_context& ioCtx, const nlohmann::json& opts) :
	Webhook(ioCtx, opts.at("webhookPort").get<unsigned short>()),
	token(opts.at("webhookToken").get<std::string>()),
	remote(opts.at("remote").get<std::string>()),
	path(NormalizeDirPath(opts.at("path").get<std::string>())),
	repo(nullptr)
{
	if(opts.count("credentials") != 0U)
	{
		const auto& credJson = opts["credentials"];
		auto user = credJson.at("username").get<std::string>();
		auto pass = credJson.at("password").get<std::string>();
		credPtr = std::make_unique<Credentials>(std::move(user), std::move(pass));
	}
	if(!CheckIfRepoExists())
	{
		spdlog::info("Repository doesnt exist, Cloning...");
		Clone();
		return;
	}
	spdlog::info("Repository exist! Opening...");
	Git::Check(git_repository_open(&repo, path.c_str()));
	spdlog::info("Checking for updates...");
	try
	{
		Fetch();
		ResetToFetchHead();
	}
	catch(...)
	{
		git_repository_free(repo);
		throw;
	}
	spdlog::info("Updating completed!");
}

GitRepo::~GitRepo()
{
	git_repository_free(repo);
}

void GitRepo::AddObserver(IGitRepoObserver& obs)
{
	observers.emplace_back(&obs);
	if(const PathVector pv = GetTrackedFiles(); !pv.empty())
		obs.OnAdd(path, pv);
}

// private

void GitRepo::Callback(std::string_view payload)
{
	spdlog::info("Webhook triggered for repository on '{:s}'", path);
	if(payload.find(token) == std::string_view::npos)
	{
		spdlog::error("Trigger doesn't have the token");
		return;
	}
	try
	{
		Fetch();
		const GitDiff diff = GetFilesDiff();
		ResetToFetchHead();
		spdlog::info("Finished updating");
		if(!diff.removed.empty() || !diff.added.empty())
			for(auto& obs : observers)
				obs->OnDiff(path, diff);
	}
	catch(const std::exception& e)
	{
		spdlog::error("Exception ocurred while updating repo: {:s}", e.what());
	}
}

bool GitRepo::CheckIfRepoExists() const
{
	return git_repository_open_ext(
		nullptr,
		path.c_str(),
		GIT_REPOSITORY_OPEN_NO_SEARCH,
		nullptr) == 0;
}

void GitRepo::Clone()
{
	// git clone <url>
	git_clone_options cloneOpts = GIT_CLONE_OPTIONS_INIT;
	if(credPtr)
	{
		cloneOpts.fetch_opts.callbacks.credentials = &CredCb;
		cloneOpts.fetch_opts.callbacks.payload = credPtr.get();
	}
	Git::Check(git_clone(&repo, remote.c_str(), path.c_str(), &cloneOpts));
	spdlog::info("Cloning completed!");
}

void GitRepo::Fetch()
{
	// git fetch
	git_fetch_options fetchOpts = GIT_FETCH_OPTIONS_INIT;
	if(credPtr)
	{
		fetchOpts.callbacks.credentials = &CredCb;
		fetchOpts.callbacks.payload = credPtr.get();
	}
	auto remote = Git::MakeUnique(git_remote_lookup, repo, "origin");
	Git::Check(git_remote_fetch(remote.get(), nullptr, &fetchOpts, nullptr));
}

void GitRepo::ResetToFetchHead()
{
	// git reset --hard FETCH_HEAD
	git_oid oid;
	Git::Check(git_reference_name_to_id(&oid, repo, "FETCH_HEAD"));
	auto commit = Git::MakeUnique(git_commit_lookup, repo, &oid);
	Git::Check(git_reset(repo, reinterpret_cast<git_object*>(commit.get()),
	                     GIT_RESET_HARD, nullptr));
}

GitDiff GitRepo::GetFilesDiff() const
{
	// git diff ..FETCH_HEAD
	auto FileCb = [](const git_diff_delta* delta, float /*unused*/, void* payload) -> int
	{
		auto& diff = *static_cast<GitDiff*>(payload);
		if(git_oid_iszero(&delta->old_file.id) == 1)
		{
			diff.added.emplace_back(delta->new_file.path);
		}
		else if(git_oid_iszero(&delta->new_file.id) == 1)
		{
			diff.removed.emplace_back(delta->old_file.path);
		}
		else
		{
			diff.removed.emplace_back(delta->old_file.path);
			diff.added.emplace_back(delta->new_file.path);
		}
		return 0;
	};
	auto obj1 = Git::MakeUnique(git_revparse_single, repo, "HEAD");
	auto obj2 = Git::MakeUnique(git_revparse_single, repo, "FETCH_HEAD");
	auto t1 = Git::Peel<git_tree>(std::move(obj1));
	auto t2 = Git::Peel<git_tree>(std::move(obj2));
	auto obj3 = Git::MakeUnique(git_diff_tree_to_tree, repo, t1.get(), t2.get(), nullptr);
	GitDiff diff;
	Git::Check(git_diff_foreach(obj3.get(), FileCb, nullptr, nullptr, nullptr, &diff));
	return diff;
}

PathVector GitRepo::GetTrackedFiles() const
{
	// git ls-files
	PathVector pv;
	auto index = Git::MakeUnique(git_repository_index, repo);
	const std::size_t entryCount = git_index_entrycount(index.get());
	const git_index_entry* entry = nullptr;
	for(std::size_t i = 0; i < entryCount; i++)
	{
		entry = git_index_get_byindex(index.get(), i);
		pv.emplace_back(entry->path);
	}
	return pv;
}

} // namespace Ignis::Multirole
