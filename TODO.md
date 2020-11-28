# TODO
* Finish README
* Prepare windows workflow build
* Once signaled to quit, make sure no more duels can be made (that could increment replay count)
* Make replaymanager write lastId to disk on each fetch
* Review and adjust duel timers
* Log room ID and replay number when a core crashes
* Add more functionality for CoreProvider
  * Make copy of used shared library because it cannot be overwritten while being used
  * Do sanity check at startup and after updating core
  * Load and cache core version and remove compile-time version
  * Handle core not loading after webhook is triggered
    * Try to fallback to the core that was working already
* Add IPC interface for discord bot
  * Filter error messages based on SCOPE of cards used in the duel

# Wishlist
* Make `GitRepo` webhook update system optional upon construction via config file
  * Move `webhookPort` and `webhookToken` to `webhook` field and rename them `port` and `token` in the config
* Make `GitRepo` able to use local repositories, either without cloning or cloning locally
* Check used enums and try to use `enum class` where possible
  * Also try to forward declare as many of them as possible
