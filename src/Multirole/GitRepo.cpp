#include "GitRepo.hpp"

#include <type_traits>
#include <tuple>

#include <git2.h>
#include <fmt/printf.h>

#include "IAsyncLogger.hpp"
#include "IGitRepoObserver.hpp"

namespace Git
{

namespace Detail
{

// Template-based interface to remove all pointers from a type T
template <typename T>
struct Identity
{
	using type = T;
};

template<typename T>
struct RemoveAllPointers : std::conditional_t<std::is_pointer_v<T>,
	RemoveAllPointers<std::remove_pointer_t<T>>, Identity<T>>
{};

template<typename T>
using RemoveAllPointers_t = typename RemoveAllPointers<T>::type;

// Template-based interface to query argument Ith type from a function pointer
template<std::size_t I, typename Sig>
struct GetArg;

template<std::size_t I, typename Ret, typename... Args>
struct GetArg<I, Ret(*)(Args...)>
{
	using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
};

template<std::size_t I, typename Sig>
using GetArg_t = typename GetArg<I, Sig>::type;

// Template-based interface to deduce a libgit object destructor from T
template<typename T>
struct DtorType;

template<>
struct DtorType<git_object>
{
	using type = void(&)(git_object*);
	static constexpr type value = git_object_free;
};

template<>
struct DtorType<git_diff>
{
	using type = void(&)(git_diff*);
	static constexpr type value = git_diff_free;
};

template<>
struct DtorType<git_remote>
{
	using type = void(&)(git_remote*);
	static constexpr type value = git_remote_free;
};

template<>
struct DtorType<git_tree>
{
	using type = void(&)(git_tree*);
	static constexpr type value = git_tree_free;
};

template<>
struct DtorType<git_repository>
{
	using type = void(&)(git_repository*);
	static constexpr type value = git_repository_free;
};

template<typename T>
using DtorType_t = typename DtorType<T>::type;

template<typename T>
inline constexpr DtorType_t<T> DtorType_v = DtorType<T>::value;

// Template-based interface to deduce a git_otype enum value from T
template<typename T>
struct TypeEnum;

template<>
struct TypeEnum<git_tree> : std::integral_constant<git_otype, GIT_OBJ_TREE>
{};

template<typename T>
inline constexpr git_otype TypeEnum_v = TypeEnum<T>::value;

// libgit generic object wrapped on a std::unique_ptr
using UniqueObjPtr = std::unique_ptr<git_object, DtorType_t<git_object>>;

} // namespace Detail

// Check error value and throw in case there is an error
inline void Check(int error)
{
	if(error >= 0)
		return;
	const git_error* e = giterr_last();
	throw std::runtime_error(fmt::format("Git: {}/{} -> {}", error, e->klass, e->message));
}

// Helper function to create RAII-managed objects for libgit C objects
template<typename Ctor,
	typename T = Detail::RemoveAllPointers_t<Detail::GetArg_t<0, Ctor>>,
	typename... Args>
decltype(auto) MakeUnique(Ctor ctor, Args&& ...args)
{
	T* obj;
	Check(ctor(&obj, std::forward<Args>(args)...));
	using namespace Detail;
	return std::unique_ptr<T, DtorType_t<T>>(std::move(obj), DtorType_v<T>);
}

// Helper function to peel a generic object into a specific libgit type
template<typename T, git_otype BT = Detail::TypeEnum_v<T>>
decltype(auto) Peel(Detail::UniqueObjPtr objPtr)
{
	T* obj;
	Check(git_object_peel(reinterpret_cast<git_object**>(&obj), objPtr.get(), BT));
	using namespace Detail;
	return std::unique_ptr<T, DtorType_t<T>>(std::move(obj), DtorType_v<T>);
}

} // namespace Git

namespace Ignis
{

namespace Multirole
{

int CredCb(git_cred** out, const char*, const char*, unsigned int aTypes, void* pl)
{
	if(!(GIT_CREDTYPE_USERPASS_PLAINTEXT & aTypes))
		return GIT_EUSER;
	const auto& cred = *static_cast<GitRepo::Credentials*>(pl);
	return git_cred_userpass_plaintext_new(out, cred.first.c_str(), cred.second.c_str());
}

std::string NormalizePath(std::string_view str)
{
	std::string tmp(str);
	if(tmp.back() != '/' || tmp.back() != '\\')
		tmp.push_back('/');
	return tmp;
}

// public

GitRepo::GitRepo(asio::io_context& ioCtx, IAsyncLogger& l, const nlohmann::json& opts) :
	Webhook(ioCtx, opts.at("webhookPort").get<unsigned short>()),
	logger(l),
	token(opts.at("webhookToken").get<std::string>()),
	remote(opts.at("remote").get<std::string>()),
	path(NormalizePath(opts.at("path").get<std::string>())),
	repo(nullptr)
{
	if(opts.count("credentials"))
	{
		const auto& credJson = opts["credentials"];
		auto user = credJson.at("username").get<std::string>();
		auto pass = credJson.at("password").get<std::string>();
		credPtr = std::make_unique<Credentials>(std::move(user), std::move(pass));
	}
	if(!CheckIfRepoExists())
	{
		// TODO: probably clean-up the directory just in case
		Clone();
		return;
	}
	fmt::print("Repository exist! Opening...\n");
	Git::Check(git_repository_open(&repo, path.c_str()));
	fmt::print("Checking for updates...\n");
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
}

GitRepo::~GitRepo()
{
	git_repository_free(repo);
}

void GitRepo::AddObserver(IGitRepoObserver& obs)
{
	observers.emplace_back(&obs);
	obs.OnAdd();
}

// private

void GitRepo::Callback(std::string_view payload)
{
	logger.Log(fmt::format("Webhook triggered for repository on '{}'", path));
	if(payload.find(token) == std::string_view::npos)
	{
		logger.LogError("Trigger doesn't have the token");
		return;
	}
	try
	{
		Fetch();
		for(const auto& s : GetFilesDiff())
			logger.Log(s);
		ResetToFetchHead();
	}
	catch(const std::exception& e)
	{
		logger.LogError(fmt::format("Exception ocurred while updating repo: {}", e.what()));
	}
	logger.Log("Finished updating");
	// TODO: inform registered observers
}

bool GitRepo::CheckIfRepoExists() const
{
	git_repository* tmp = nullptr;
	const int status = git_repository_open_ext(&tmp, path.c_str(), GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr);
	git_repository_free(tmp);
	return status == 0;
}

void GitRepo::Clone()
{
	// git clone <url>
	git_clone_options cloneOpts = GIT_CLONE_OPTIONS_INIT;
	cloneOpts.fetch_opts.callbacks.transfer_progress =
	[](const git_transfer_progress* stats, void*) -> int
	{
		int percent;
		if(stats->received_objects != stats->total_objects)
			percent = (75 * stats->received_objects) / stats->total_objects;
		else if(stats->total_deltas == 0)
			percent = 75;
		else
			percent = 75 + ((25 * stats->indexed_deltas) / stats->total_deltas);
		fmt::print("\rCloning... {}%", percent);
		fflush(stdout);
		return GIT_OK;
	};
	if(credPtr)
	{
		cloneOpts.fetch_opts.callbacks.credentials = &CredCb;
		cloneOpts.fetch_opts.callbacks.payload = credPtr.get();
	}
	fmt::print("Cloning...");
	Git::Check(git_clone(&repo, remote.c_str(), path.c_str(), &cloneOpts));
	fmt::print("\n");
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

std::vector<std::string> GitRepo::GetFilesDiff() const
{
	// git diff ..FETCH_HEAD
	std::vector<std::string> list;
	auto FileCb = [](const git_diff_delta* delta, float, void* payload) -> int
	{
		auto& list = *static_cast<std::vector<std::string>*>(payload);
		list.emplace_back(delta->new_file.path);
		return 0;
	};
	auto obj1 = Git::MakeUnique(git_revparse_single, repo, "HEAD");
	auto obj2 = Git::MakeUnique(git_revparse_single, repo, "FETCH_HEAD");
	auto t1 = Git::Peel<git_tree>(std::move(obj1));
	auto t2 = Git::Peel<git_tree>(std::move(obj2));
	auto diff = Git::MakeUnique(git_diff_tree_to_tree, repo, t1.get(), t2.get(), nullptr);
	Git::Check(git_diff_foreach(diff.get(), FileCb, nullptr, nullptr, nullptr, &list));
	return list;
}

void GitRepo::ResetToFetchHead()
{
	// git reset --hard FETCH_HEAD
	// TODO
}

} // namespace Multirole

} // namespace Ignis
