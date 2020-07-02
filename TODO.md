# TODO

* Implement sidedecking
* Implement spectators connecting mid-duel
  * Make sure they are properly added and removed from `spectators` set
* Implement timers
* Implement replays
* Implement "Hornet" core-type (crash resilient implementation of `Core::IWrapper`)
* Handle crashes of core
* Add IPC interface for discord bot

# Wishlist

* Make `GitRepo` webhook update system optional upon construction via config file
  * Move `webhookPort` and `webhookToken` to `webhook` field and rename them `port` and `token` in the config
* Check used enums and try to use `enum class` where possible
  * Also try to forward declare as many of them as possible
* Make inheritance of `std::enable_shared_from_this` private/protected
* Conditionally be able to send messages stored on a smart pointer instead of doing possibly expensive copies of the message
  * Needed mainly for Replays
* Remove setters for `Core::IWrapper` and instead set them on ctors
* Investigate possibility of using a lockless queue for `Room::Client` message sending
* Separate parts of `STOCMsgFactory` that depend on Client from the parts that do not
* Instead of using Client::POSITION_SPECTATOR make client position be `std::optional<PosType>`
  * Lack of value would represent a spectator

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
