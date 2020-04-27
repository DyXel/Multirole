# dependencies
`sqlite3 fmt asio spdlog libgit2`

# other
The server guarantees 3 things
1) after initialization has finished, no disk read will be done except if loadPerRoom on the coreprovider is set to true, or if an update is triggered (either with the webhooks, yourself with curl, and in the future with the bot)
2) after you signal the server to close, it will not do any more disks reads (even if loadPerRoom is set to true), and all acceptors will be closed so you can launch another instance of the server immediately (zero downtime).
3) No disk write will ever be done, except by the replay manager in the future (write atomic value of last replay id, and write the replays ofc)
