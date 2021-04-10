#include "GitRepo.hpp"

#include <boost/filesystem.hpp>
#include <boost/json/value.hpp>

#include "I18N.hpp"
#include "IGitRepoObserver.hpp"
#include "libgit2.hpp"
#include "Service/LogHandler.hpp"
#define LOG_INFO(...) lh.Log(ServiceType::GIT_REPO, Level::INFO, __VA_ARGS__)
#define LOG_ERROR(...) lh.Log(ServiceType::GIT_REPO, Level::ERROR, __VA_ARGS__)

namespace Ignis::Multirole
{

namespace
{

int CredCb(git_cred** out, const char* /*unused*/, const char* /*unused*/, unsigned int aTypes, void* pl)
{
	if((GIT_CREDTYPE_USERPASS_PLAINTEXT & aTypes) == 0U)
		return GIT_EUSER;
	const auto& cred = *static_cast<GitRepo::Credentials*>(pl);
	return git_cred_userpass_plaintext_new(out, cred.first.c_str(), cred.second.c_str());
}

} // namespace

// public

GitRepo::GitRepo(Service::LogHandler& lh, boost::asio::io_context& ioCtx, const boost::json::value& opts) :
	Webhook(ioCtx, opts.at("webhookPort").to_number<unsigned short>()),
	lh(lh),
	token(opts.at("webhookToken").as_string().data()),
	remote(opts.at("remote").as_string().data()),
	path(opts.at("path").as_string().data()),
	repo(nullptr)
{
	if(const auto* const cred = opts.as_object().if_contains("credentials"); cred)
	{
		credPtr = std::make_unique<Credentials>(
			cred->at("username").as_string().data(),
			cred->at("password").as_string().data());
	}
	if(boost::filesystem::exists(path) && !boost::filesystem::is_directory(path))
		throw std::runtime_error(I18N::GIT_REPO_PATH_IS_NOT_DIR);
	if(!CheckIfRepoExists())
	{
		LOG_INFO(I18N::GIT_REPO_DOES_NOT_EXIST);
		Clone();
		return;
	}
	LOG_INFO(I18N::GIT_REPO_EXISTS);
	Git::Check(git_repository_open(&repo, path.string().data()));
	LOG_INFO(I18N::GIT_REPO_CHECKING_UPDATES);
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
	LOG_INFO(I18N::GIT_REPO_UPDATE_COMPLETED);
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
	LOG_INFO(I18N::GIT_REPO_WEBHOOK_TRIGGERED, path.string());
	if(payload.find(token) == std::string_view::npos)
	{
		LOG_ERROR(I18N::GIT_REPO_WEBHOOK_NO_TOKEN);
		return;
	}
	try
	{
		Fetch();
		const GitDiff diff = GetFilesDiff();
		ResetToFetchHead();
		LOG_INFO(I18N::GIT_REPO_FINISHED_UPDATING);
		if(!diff.removed.empty() || !diff.added.empty())
			for(auto& obs : observers)
				obs->OnDiff(path, diff);
	}
	catch(const std::exception& e)
	{
		LOG_ERROR(I18N::GIT_REPO_UPDATE_EXCEPT, e.what());
	}
}

bool GitRepo::CheckIfRepoExists() const
{
	return git_repository_open_ext(
		nullptr,
		path.string().data(),
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
	Git::Check(git_clone(&repo, remote.c_str(), path.string().data(), &cloneOpts));
	LOG_INFO(I18N::GIT_REPO_CLONING_COMPLETED);
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
