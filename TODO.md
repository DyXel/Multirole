# TODO

* Keep repositories closed and only open them when performing operations
  * Should drastically reduce the file descriptor count over prolonged use

# Wishlist
* Make logging mechanism entirely disableable as compile configuration
* Make `GitRepo` webhook update system optional upon construction via config file
  * Move `webhookPort` and `webhookToken` to `webhook` field and rename them `port` and `token` in the config
* Make `GitRepo` able to use local repositories, either without cloning or cloning locally
* Implement anti-stalling measures
* Review places where file handles can be opened and check for their errors
  * An idea would be to artifically lower the limit in order to test places randomly
* Limit number of messages/memory a particular room can have allocated
  * Use all collected replays to calculate a suitable default max
* CoreProvider: Load and cache core version and remove compile-time version
