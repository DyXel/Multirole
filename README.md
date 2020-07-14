# dependencies
`sqlite3 fmt asio spdlog libgit2`

# other
The server guarantees 3 things:
  1. After initialization has finished, no disk read will be done except if `loadPerRoom` on the coreProvider is set to `true`, or if an update is triggered (either with the webhooks, yourself with curl, and in the future with the bot)
  2. After you signal the server to close, it will not do any more disks reads (even if `loadPerRoom` is set to true), and all acceptors will be closed so you can launch another instance of the server immediately (zero downtime).
  3. No disk write will ever be done, except by the replay manager in the future (write atomic value of last replay id, and write the replays ofc)

After running for a while, libgit operations start failing with error `Too many open files`; This is a known issue, fixed upstream: https://github.com/libgit2/libgit2/pull/5386
  * A workaround (if you are stuck with packager's version) is raising the limit of open files Multirole can have, see the issue linked by the above PR for details
