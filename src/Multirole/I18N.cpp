#include "I18N.hpp"

namespace Ignis::Multirole::I18N
{

Str GIT_REPO_PATH_IS_NOT_DIR = "Repository path is not a directory.";
Str GIT_REPO_DOES_NOT_EXIST = "Repository does not exist, cloning...";
Str GIT_REPO_EXISTS = "Repository exists! Opening...";
Str GIT_REPO_CHECKING_UPDATES = "Checking for updates...";
Str GIT_REPO_UPDATE_COMPLETED = "Update completed!";
Str GIT_REPO_WEBHOOK_TRIGGERED = "Webhook triggered.";
Str GIT_REPO_WEBHOOK_NO_TOKEN = "Webhook payload does not have a token.";
Str GIT_REPO_FINISHED_UPDATING = "Finished updating.";
Str GIT_REPO_UPDATE_EXCEPT = "Exception occurred while updating repo: {0}";
Str GIT_REPO_CLONING_COMPLETED = "Cloning completed!";

Str MULTIROLE_INCORRECT_CORE_TYPE = "Incorrect type of core.";
Str MULTIROLE_ADDING_REPO = "Adding repository '{0}'...";
Str MULTIROLE_SETUP_SIGNAL = "Setting up signal handling...";
Str MULTIROLE_SIGNAL_RECEIVED = "SIGTERM received.";
Str MULTIROLE_HOSTING_THREADS_NUM = "Hosting will use {0} threads.";
Str MULTIROLE_INIT_SUCCESS = "Initialization finished successfully!";
Str MULTIROLE_CLEANING_UP = "Closing acceptors and repositories...";
Str MULTIROLE_UNFINISHED_DUELS =
"All done, server will gracefully finish execution "
"after all duels finish. If you wish to forcefully end "
"you can terminate the process safely now (SIGTERM/SIGKILL). "
"Remaining duels: {0}";

Str MAIN_SERVER_INIT_FAILURE = "Could not initialize server: {0}\n";

Str DLWRAPPER_EXCEPT_CREATE_DUEL = "OCG_CreateDuel failed!";

Str HWRAPPER_UNABLE_TO_LAUNCH = "Unable to launch child.";
Str HWRAPPER_HEARTBEAT_FAILURE = "Heartbeat failed.";
Str HWRAPPER_EXCEPT_CREATE_DUEL = DLWRAPPER_EXCEPT_CREATE_DUEL;
Str HWRAPPER_EXCEPT_MAX_LOOP_COUNT = "Max loop count reached.";
Str HWRAPPER_EXCEPT_PROC_CRASHED = "Process is not running.";
Str HWRAPPER_EXCEPT_PROC_UNRESPONSIVE = "Process is unresponsive.";

Str CLIENT_ROOM_HOSTING_INVALID_NAME = "Invalid name. Try filling in your name.";
Str CLIENT_ROOM_HOSTING_NOT_FOUND = "Room not found. Try refreshing the list!";
Str CLIENT_ROOM_HOSTING_INVALID_MSG =
"Invalid message before connecting to a room. Please report this error!";
Str CLIENT_ROOM_HOSTING_KICKED_BEFORE =
"Unable to join. You were kicked from this room before.";

Str CLIENT_ROOM_MSG_RETRY_ERROR =
"Error while processing your response. Make sure you have the lastest client.";

Str ROOM_DUELING_CORE_EXCEPT_CREATION =
"Core exception at creation (Replay ID: {0}): {1}";
Str ROOM_DUELING_CORE_EXCEPT_EXTRA_CARDS =
"Core exception at extra cards addition (Replay ID: {0}): {1}";
Str ROOM_DUELING_CORE_EXCEPT_STARTING =
"Core exception at starting (Replay ID: {0}): {1}";
Str ROOM_DUELING_CORE_EXCEPT_RESPONSE =
"Core exception at response setting (Replay ID: {0}): {1}";
Str ROOM_DUELING_CORE_EXCEPT_PROCESSING =
"Core exception at processing (Replay ID: {0}): {1}";
Str ROOM_DUELING_CORE_EXCEPT_DESTRUCTOR =
"Core exception at destruction (Replay ID: {0}): {1}";
Str ROOM_DUELING_MSG_RETRY_RECEIVED =
"MSG_RETRY received from core (Replay ID: {0}).";
Str CLIENT_ROOM_REPLAY_TOO_BIG =
"Unable to send replay, its size exceeds the maximum capacity.";
Str CLIENT_ROOM_CORE_EXCEPT =
"Internal scripting engine error! This incident has been reported.";

Str CLIENT_ROOM_KICKED = "{0} has been kicked.";

Str BANLIST_PROVIDER_LOADING_ONE = "BanlistProvider: Loading up {0}...";
Str BANLIST_PROVIDER_COULD_NOT_LOAD_ONE = "BanlistProvider: Could not load banlist: {0}";

Str CORE_PROVIDER_COULD_NOT_CREATE_TMP_DIR = "CoreProvider: Could not create temporary directory.";
Str CORE_PROVIDER_PATH_IS_FILE_NOT_DIR = "CoreProvider: Temporary directory path points to a file.";
Str CORE_PROVIDER_WRONG_CORE_TYPE = "CoreProvider: No other core type is implemented.";
Str CORE_PROVIDER_CORE_NOT_FOUND_IN_REPO = "CoreProvider: Core not found in repository!";
Str CORE_PROVIDER_COPYING_CORE_FILE = "CoreProvider: Copying core from '{0}' to '{1}'...";
Str CORE_PROVIDER_FAILED_TO_COPY_CORE_FILE = "CoreProvider: Failed to copy core file! Re-testing old one.";
Str CORE_PROVIDER_VERSION_REPORTED = "CoreProvider: Version reported by core: {0}.{1}";
Str CORE_PROVIDER_ERROR_WHILE_TESTING = "CoreProvider: Error while testing core '{0}': {1}";

Str DATA_PROVIDER_LOADING_ONE = "DataProvider: Loading up {0}...";
Str DATA_PROVIDER_COULD_NOT_MERGE = "DataProvider: Could not merge database.";

Str REPLAY_MANAGER_NOT_SAVING_REPLAYS = "ReplayManager: Not saving replays, replay IDs will always be 0";
Str REPLAY_MANAGER_COULD_NOT_CREATE_DIR = "ReplayManager: Could not create replay directory.";
Str REPLAY_MANAGER_PATH_IS_FILE_NOT_DIR = "ReplayManager: Replay directory path points to a file.";
Str REPLAY_MANAGER_ERROR_WRITING_INITIAL_ID = "ReplayManager: Unable to write initial ID!";
Str REPLAY_MANAGER_CURRENT_ID = "ReplayManager: Current ID is {0}.";
Str REPLAY_MANAGER_LASTID_SIZE_CORRUPTED =
"ReplayManager: lastId file byte size corrupted: It's {0}. Should be {1}.";
Str REPLAY_MANAGER_UNABLE_TO_SAVE = "ReplayManager: Unable to save replay {0}.";
Str REPLAY_MANAGER_CANNOT_OPEN_LASTID = "ReplayManager: lastId cannot be opened for reading.";
Str REPLAY_MANAGER_CANNOT_WRITE_ID = "ReplayManager: Unable to write next replay ID to file.";

Str SCRIPT_PROVIDER_LOADING_FILES = "ScriptProvider: Loading {0} files...";
Str SCRIPT_PROVIDER_COULD_NOT_OPEN = "ScriptProvider: Could not open file {0}.";
Str SCRIPT_PROVIDER_TOTAL_FILES_LOADED = "ScriptProvider: Loaded {0} files.";

} // namespace Ignis::Multirole::I18N
