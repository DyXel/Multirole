# TODO
* Make core crash not terminate the room
* Review all places where connections are terminated and make sure their termination is graceful (closed by client)
  * Webhook (Not marked as OK by github)
  * RoomHosting
  * All State(s), specially State::Closing
* Implement STOC_CHAT_2
* Implement anti-stalling measures
* Investigate top deck being moved to overlay and not showing
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
* CoreProvider: Load and cache core version and remove compile-time version.
* Search workarounds for LZMA compression allocating big blocks of memory
* Use boost througly rather than handrolled solutions
  * FileSystem.hpp == boost.FileSystem
  * Hornet core process launching mechanism == boost.Process
  * asio == Boost.Asio
  * nlohmann.Json == Boost.Json?
  * spdlog == Boost.Log?
