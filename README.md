# Multirole: The robust C++ server for EDOPro
Multirole manages client connections to a singular lobby where rooms can be hosted and a list of said rooms fetched by other clients to join. It is in charge of creating, processing and logging duels (saving their replays) by interfacing with [EDOPro's core](https://github.com/edo9300/ygopro-core).

This project's original inception was to replace [srvpro](https://github.com/mycard/srvpro) due to how hard was to work with CoffeScript/TypeScript and its interface with native data structures while doubling as an learning excercise about high performance networking and serving as a very small documentation for YGOPro's ecosystem.

## Features

  * High performance and scalability.
  * Automatic scripts, databases, banlists and core updates through remote git repositories pulling + webhook mechanism.
  * Server-side replay saving, for debug or analytic purposes.
  * Core crash resilience (EDOPro's core crashing does not propagate to the cluster).
  * Relatively easy compilation and/or deployment through docker.

## Building
A C++17 compliant compiler is needed as well as [Meson](https://mesonbuild.com/) which is the build system that Multirole uses.

This project depends on the following libraries (most of them being header-only):

  * asio (non-Boost flavor)
  * boost (only Boost.Interprocess and its dependency: Boost.DateTime)
  * fmt
  * libgit2
  * spdlog
  * sqlite3

Once you have all necessary tools and dependencies, compiling should be as simple as doing:

    meson setup build --buildtype=release
    cd build
    ninja

You can (and should) take a look at the github workflow file(s) to ease this process. You can also use the Dockerfile, which should handle everything related to building for you.

## Configuring and Running
TODO

## Remarks

  * Multirole can be signaled to close its acceptors (unbind sockets from ports) by sending SIGTERM so that another instance of it can be launched without having to terminate the current duels. Be mindful about signaling through a shell as the default behavior is to propagate the signal to child processes.

  * Depending on the version of libgit2 library used, after updating the git repositories several times, operations will start failing with `Too many open files`; This is a known issue, [fixed upstream](https://github.com/libgit2/libgit2/pull/5386). A workaround (if you are stuck with packager's version that has this issue) is raising the limit of open files Multirole can have, see the issue linked by the PR for details.

  * Multirole is able to outlive core crashes (segfaults, etc) because its interfacing mechanism involves spawning new processes where the actual core processing occurs, and communicates the data through a interprocess protocol which uses shared memory as transport layer. The program checks if that said process is running during each operation, reporting and dealing accordingly with any issue that occurs if the child process fails to communicate. Note that the reverse is not true: Should Multirole crash, the child processes will be orphaned while waiting for their parent to notify them (in this case, never). In that degenerate case the system or user is required to terminate them as they will never exit by themselves.
