# TODO
* Fix banlist being removed and replaced while rooms are still using them
  * Return smart pointer of banlists rather than normal pointer
* Add more functionality for CoreProvider
  * Make copy of used shared library because it cannot be overwritten while being used
  * Do sanity check at startup and after updating core
  * Load and cache core version and remove compile-time version
  * Handle core not loading after webhook is triggered
    * Try to fallback to the core that was working already
* Investigate Sealed Duel finishing with time up
* Implement STOC_CHAT_2
* Implement anti-stalling measures
* Fix webhooks not being marked as OK
  * Review all places where connections are terminated and make sure their termination is graceful
* Finish README
* Overhaul error logging mechanism
  * Send error messages to clients
  * Log room ID and replay number when a core crashes
  * Filter error messages based on SCOPE of cards used in the room/duels
* Prepare windows workflow build

# Wishlist
* Make `GitRepo` webhook update system optional upon construction via config file
  * Move `webhookPort` and `webhookToken` to `webhook` field and rename them `port` and `token` in the config
* Make `GitRepo` able to use local repositories, either without cloning or cloning locally
* Check used enums and try to use `enum class` where possible
  * Also try to forward declare as many of them as possible
* Search workarounds for LZMA compression allocating big blocks of memory
