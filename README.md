<p align="center">
	<img src="./logo.svg" alt="Multirole logo"/>
</p>

![x64-linux](https://github.com/ProjectIgnis/Multirole/workflows/x64-linux/badge.svg)
![x86-windows](https://github.com/ProjectIgnis/Multirole/workflows/x86-windows/badge.svg)

# Multirole: A C++ server for EDOPro

Multirole manages client connections to a singular lobby where rooms can be hosted and a list of said rooms fetched by other clients to join. It is in charge of creating, processing and logging duels by interfacing with [EDOPro's core](https://github.com/edo9300/ygopro-core).

This project's original inception was to replace [srvpro](https://github.com/mycard/srvpro) due to how cumbersome it is to work with CoffeScript/JavaScript and its interface for native data structures, while also doubling as a learning exercise about high performance networking and serving as a very small documentation for YGOPro's ecosystem.

## Features

  * High performance and scalability.
  * Core scripting engine sandboxing (Core crashes and/or unresponsiveness is handled gracefully).
  * Automatic scripts, databases, banlists and core updates through remote git repositories pulling + webhook mechanism.
  * Flexible script and core error logging mechanism.
  * Optional server-side replay saving, for debug or analytic purposes.
  * Optional room notes and chat logging, for moderation purposes.
  * Easy compilation and deployment through docker with very small container image.

## Building

A C++17 compliant compiler is needed as well as [Meson](https://mesonbuild.com/) which is the build system that Multirole uses.

This project depends on the following libraries:

  * boost
    * asio
    * filesystem (requires linking)
    * interprocess (requires Boost.datetime)
    * json
  * fmt
  * libgit2
  * openssl
  * sqlite3

Once you have all necessary tools and dependencies, compiling should be as simple as doing:

```sh
# For Linux and other POSIX-oriented OS
meson setup build && meson compile -C build
# For Windows
meson setup build --vsenv --backend=vs
meson compile -C build # Alternatively, open the generated solution and build it
```

You should take a look at the github workflow files to learn how to setup the development environment for your platform. You can also use the Dockerfile, which should handle everything related to building for you.

## Configuring and Running

Once you have built everything you should configure the environment in which the server will run as well as the server itself to suit your needs, the [default configuration file](https://github.com/DyXel/Multirole/blob/master/etc/config.json) works fine if you just want to run a server instance as quickly as possible by using [Project Ignis](https://github.com/ProjectIgnis/)' banlists, databases, scripts, as well as EDOPro's core.

The configuration file must be placed in the same working directory as Multirole and must be called `config.json`. Here is a non-exhaustive explanation of the settings that Multirole reads from said file, for more details please read the source code, starting from [here](https://github.com/DyXel/Multirole/blob/master/src/Multirole/Instance.cpp):

  * `concurrencyHint`: Number of threads that will be used by the room's asynchronous handling, putting a negative value lets Multirole decide the amount, which is usually the machine's CPU cores times 2.

  * `lobbyListingPort`: Port that will be used by the client to fetch the server's room list.

  * `lobbyMaxConnections`: Maximum number of connections a single IP can have to the lobby. Any negative value disables this check.

  * `roomHostingPort`: Port that will be used by the client to host new rooms, or to join rooms that were previously fetched.

  * `repos`: An array of repositories settings that will be cloned and synchronized for usage by Multirole's services, each repository object must have the following fields:

    * `name`: Unique identifier, used by the services to know from which repo to pull files from.

    * `remote`: Where from synchronize this repository as if by doing `git fetch && git reset --hard FETCH_HEAD`.

    * `path`: Where to save the repository in the local filesystem.

    * `webhookPort` and `webhookToken`: Port and token used by the webhook updating mechanism, it is enough doing `curl -X POST http://localhost:<PORT>/<TOKEN>` to trigger an update. NOTE: These fields are not optional, and each port must not be in use.

  * `banlistProvider`: `Service::BanlistProvider` settings, the service that provides banlists objects to each room:

    * `observedRepos`: Array of repositories' names where banlist files will be fetched from.

    * `fileRegex`: Regular expression that will match or discard files to load.

  * `coreProvider`: `Service::CoreProvider` settings, the service that provides a working core interface object to each room:

    * `observedRepos`: Array of repositories' names where shared object files or DLL files will be fetched from.

    * `fileRegex`: Regular expression that will match or discard files to load.

    * `tmpPath`: Path to a temporary directory where the picked file will be copied to, used to prevent repository updates from modifying it. If the directory doesn't exist, it'll be created non-recursively.

    * `coreType`: The type of core interface object to be used for all rooms, must be `"hornet"` or `"shared"`. `hornet` is the default and isolates each core to its own process, trading performance for stability. `shared` directly loads the shared object/DLL in the main process, providing maximum performance at the cost of stability.

    * `loadPerRoom`: Flag that decides if a core interface object is loaded per each room. For each `hornet` this allows each room to fail in a individual basis instead of bringing every duel down. For `shared` this settings is mostly useless as the core crashing will just make the entire server crash anyways.

  * `dataProvider`: `Service::DataProvider` settings, the service that provides card databases and information to each room:

    * `observedRepos`: Array of repositories' names where database files will be fetched from.

    * `fileRegex`: Regular expression that will match or discard files to load.

  * `logHandler`: `Service::LogHandler` settings, the service that is in charge of logging data for the entire program:

    * `serviceSinks` and `ecSinks`: List of sink types and settings for each output that the server can use. Sinks are not optional but their type can be set to `"null"` to disable logging for that service/category. There are several sink names, check the default configuration file for each one. Here is the list of each sink type along their properties:

      * `"discordWebhook"`: Logs messages to a [Discord Webhook](https://support.discord.com/hc/en-us/articles/228383668-Intro-to-Webhooks):

        * `uri`: URL where log calls will be HTTP POST'd to. Check Discord's documentation for more details.

        * `ridFormat`: Format string used to display the Replay ID of the error shown. Defaults to `"\nReplay ID: {0}"`, if not defined.

      * `"file"`: Log messages to a particular file:

        * `path`: Which file to log to, will append the text output to it. NOTE: The file will be created if it doesn't exist, but the directory will not be created.

      * `"stdout"` and `"stderr"`: Log messages to standard output or standard error pipes, respectively.

    * `roomLogging`: Options for per room logging:

      * `enabled`: Self-explanatory.

      * `path`: Path to a directory where room log files will be saved to. If the directory doesn't exist, it'll be created non-recursively.

  * `replayManager`: `Service::ReplayManager` settings, the service that provides replay ids and saves replays in the local filesystem:

    * `save`: Flag to decide if replays should be saved or not to the filesystem.

    * `path`: Path to a directory where replays are saved. If the directory doesn't exist, it'll be created non-recursively.

  * `scriptProvider`: `Service::ScriptProvider` settings, the service that loads and provides card scripts to each room:

    * `observedRepos`: Array of repositories' names where script files will be fetched from.

    * `fileRegex`: Regular expression that will match or discard files to load.

## Remarks

  * Multirole uses the TCP keepalive probing mechanism, so it is strongly recommended to configure the system-wide timers and retries for it to be relatively low in order to avoid too many dead connections preventing Multirole from doing internal memory cleanups.

  * Multirole registers a handle to capture the SIGTERM signal to close its acceptors and free repositories locks so that another instance can be launched without having to terminate the current duels ([area-zero.py](https://github.com/DyXel/Multirole/blob/master/util/area-zero.py) provides an easy way of "restarting" Multirole with no down time by using this signal handling).

  * Server users might want to raise the number of file descriptors Multirole can have open as the default system-wide amount is too low for very high volume of clients, also, depending on the version of libgit2 library used, after updating git repositories several times, operations will start failing with `Too many open files`; This is a known issue, [fixed upstream](https://github.com/libgit2/libgit2/pull/5386). See the issue linked by the PR for details on how to raise those limits.

  * Multirole is able to outlive core crashes (segfaults, etc) because its interfacing mechanism involves spawning new processes where the actual core processing occurs, and communicates the data through a interprocess protocol which uses shared memory as transport layer. The program checks if said process is running during each operation, reporting and dealing accordingly with any issue that occurs if the child process fails to communicate. Note that the reverse is not true: Should Multirole crash, the child processes will be orphaned while waiting for their parent to notify them (in this case, never). In that degenerate case the system or user is required to terminate them as they will never exit by themselves (this is not necessary on Linux-based systems, as Hornet sets the parent-death signal to SIGKILL).
