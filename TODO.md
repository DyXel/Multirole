# TODO

* Make `GitRepo` webhook update system optional upon construction via config file
   * Move `webhookPort` and `webhookToken` to `webhook` field and rename them `port` and `token` in the config
* Check used enums and try to use `enum class` where possible
  * Also try to forward declare as many of them as possible
* Make inheritance of `std::enable_shared_from_this` private/protected
* Use mutable lambda and move semantics instead of smart pointers where possible
  * https://stackoverflow.com/a/47698904
* Conditionally be able to send messages stored on a smart pointer instead of doing possibly expensive copies of the message
  * Needed mainly for Replays
* Remove setters for `Core::IHighLevelWrapper` and instead set them on ctors
* Investigate possibility of using a lockless queue for `Room::Client` message sending
* use `asio::post` and strand to dispatch Join event, avoids multithreading issues
 * maybe merge RegisterToOwner and Start into a single function that is dispatched once
* Separate parts of `STOCMsgFactory` that depend on Client from the parts that do not

# Porting to YGOpen project

## Needed changes to make a compatible server
1. Use `protobuf` described objects instead of hand-made `STOC msgs`
   * `banlist.proto`
   * `deck.proto`
   * etc...

## Medium changes
1. Move `Ignis::Multirole::Core` to `YGOpen::Core`
   * `DynamicLinkWrapper.*` -> `dynamic_link_wrapper.*`
   * `IDataSupplier.hpp` -> `data_supplier.hpp`
   * `IHighLevelWrapper.hpp` -> `high_level_wrapper.hpp`
   * `ILogger.hpp` -> `logger.hpp`
   * `IScriptSupplier.hpp` -> `script_supplier.hpp`
2. Move and rename `src/Multirole/CardDatabase.*` to `src/card_database.*`
   * Use "enums/type.hpp" instead of `TYPE_LINK` constant in `card_database.cpp`
