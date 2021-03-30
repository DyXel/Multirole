# TODO
* Update README
  * Remark that "Too Many Open Files" error not only affects the update mechanism and should be of high priority if dealing with a high server load, as each hornet and replay saving also consumes file descriptors
  * Write configuration section (config.json structure), explaining every setting.
* Prepare windows workflow build

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
