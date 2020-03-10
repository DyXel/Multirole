# TODO
* Reload all databases on a different amalgamation upon triggering update (needed to remove old beta cards)
   * Make `GitRepo` report removed files as well as added/updated ones
* Move `GitRepo`'s `libgit2` "abstraction layer" to a individual file
* Close/Destroy instance's `GitRepo`s when signal is received (needed to allow other instance to launch properly)

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
2. Move and rename `src/Multirole/SqliteDatabase.*` to `src/sqlite_database.*`
3. Move and rename `src/Multirole/CardDatabase.*` to `src/card_database.*`
   * Use "enums/type.hpp" instead of `TYPE_LINK` constant in `card_database.cpp`
