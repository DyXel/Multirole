# TODO
* Make ReplayManager write lastId to disk on each fetch
* Fix webhooks not being marked as OK
  * Review all places where connections are terminated and make sure their termination is graceful
* Review and adjust duel timers
* Finish README
* Add IPC interface for discord bot
  * Filter error messages based on SCOPE of cards used in the duel
  * Log room ID and replay number when a core crashes
* Add more functionality for CoreProvider
  * Make copy of used shared library because it cannot be overwritten while being used
  * Do sanity check at startup and after updating core
  * Load and cache core version and remove compile-time version
  * Handle core not loading after webhook is triggered
    * Try to fallback to the core that was working already
* Prepare windows workflow build
* Fix memory fragmentation issues on systems that are unable to move physical pages for program
  * Affects VPS
  * Try to optimize allocations by using massif to analyze memory's behavior
  * A simple workaround would be to restart multirole after running for a while

# Wishlist
* Make `GitRepo` webhook update system optional upon construction via config file
  * Move `webhookPort` and `webhookToken` to `webhook` field and rename them `port` and `token` in the config
* Make `GitRepo` able to use local repositories, either without cloning or cloning locally
* Check used enums and try to use `enum class` where possible
  * Also try to forward declare as many of them as possible
