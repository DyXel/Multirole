# TODO
* Implement STOC_CHAT_2
* Implement anti-stalling measures
  * Try to use TCP keep-alive (necessary when timers are unused)
* Finish README
  * Remark that "Too Many Open Files" error not only affects the update mechanism and should be of high priority if dealing with a high server load, as each hornet and replay saving also consumes file descriptors
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
* Review places where file handles can be opened and check for their errors
  * An idea would be to artifically lower the limit in order to test places randomly
* Limit number of messages/memory a particular room can have allocated
  * Use all collected replays to calculate a suitable default max
* CoreProvider: Load and cache core version and remove compile-time version
* Use boost througly rather than handrolled solutions
  * FileSystem.hpp == boost.FileSystem
  * Hornet core process launching mechanism == boost.Process
  * asio == Boost.Asio
  * nlohmann.Json == Boost.Json?
  * spdlog == Boost.Log?
