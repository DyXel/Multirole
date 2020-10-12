# TODO

* Implement "Hornet" core-type (crash resilient implementation of `Core::IWrapper`)
  * Simple "stream-less" implementation as proof-of-concept
  * Do parameter streaming for big enough strings
  * Design exceptions
  * Detect if the process is dead before attempting to do IPC
* Handle crashes of core
* Deal with core not loading after webhook is triggered
  * Try to fallback to the core that was working already
* Fix names being swapped on replay
  * Only happens on tag mode, for the players who went second
* Add IPC interface for discord bot

# Wishlist

* Make `GitRepo` webhook update system optional upon construction via config file
  * Move `webhookPort` and `webhookToken` to `webhook` field and rename them `port` and `token` in the config
* Check used enums and try to use `enum class` where possible
  * Also try to forward declare as many of them as possible
* Try to avoid including the banlist parser into BanlistProvider
* Investigate possibility of using a lockless queue for `Room::Client` message sending
* Separate parts of `STOCMsgFactory` that depend on Client from the parts that do not

# Porting to YGOpen Project

## Needed changes to make a compatible server
1. Use `protobuf` described objects instead of hand-made `STOC msgs`
   * `banlist.proto`
   * `deck.proto`
   * etc...

## Medium changes
1. Move `Ignis::Multirole::Core` to `YGOpen::Core`
   * `DLWrapper.*` -> `dlwrapper.*`
   * `IDataSupplier.hpp` -> `data_supplier.hpp`
   * `IWrapper.hpp` -> `wrapper.hpp`
   * `ILogger.hpp` -> `logger.hpp`
   * `IScriptSupplier.hpp` -> `script_supplier.hpp`
2. Move and rename `src/Multirole/CardDatabase.*` to `src/card_database.*`
   * Use "enums/type.hpp" instead of `TYPE_LINK` constant in `card_database.cpp`
