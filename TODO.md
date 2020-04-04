# TODO
* Make `GitRepo` webhook update system optional upon construction via config file
   * Move `webhookPort` and `webhookToken` to `webhook` field and rename them `port` and `token` in the config
* Implement custom move assignment operator for `GitRepo`
* Check used enums and try to use `enum class` where possible
* Make inheritance of `std::enable_shared_from_this` private/protected
* Check where `std::move` is used to "steal" data and use rvalue ref instead of passing by value

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
