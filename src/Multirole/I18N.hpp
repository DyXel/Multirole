#ifndef MULTIROLE_I18N_HPP
#define MULTIROLE_I18N_HPP

// NOTE: strings that start with CLIENT_ are sent to connected clients.

namespace Ignis::Multirole::I18N
{

using Str = const char* const;

extern Str GIT_REPO_PATH_IS_NOT_DIR;
extern Str GIT_REPO_DOES_NOT_EXIST;
extern Str GIT_REPO_EXISTS;
extern Str GIT_REPO_CHECKING_UPDATES;
extern Str GIT_REPO_UPDATE_COMPLETED;
extern Str GIT_REPO_WEBHOOK_TRIGGERED;
extern Str GIT_REPO_WEBHOOK_NO_TOKEN;
extern Str GIT_REPO_FINISHED_UPDATING;
extern Str GIT_REPO_UPDATE_EXCEPT;
extern Str GIT_REPO_CLONING_COMPLETED;

extern Str MULTIROLE_INCORRECT_CORE_TYPE;
extern Str MULTIROLE_ADDING_REPO;
extern Str MULTIROLE_SETUP_SIGNAL;
extern Str MULTIROLE_SIGNAL_RECEIVED;
extern Str MULTIROLE_HOSTING_THREADS_NUM;
extern Str MULTIROLE_INIT_SUCCESS;
extern Str MULTIROLE_CLEANING_UP;
extern Str MULTIROLE_UNFINISHED_DUELS;

extern Str MAIN_SERVER_INIT_FAILURE;

extern Str DLWRAPPER_EXCEPT_CREATE_DUEL;

// NOTE: HWRAPPER == HORNET_WRAPPER
extern Str HWRAPPER_UNABLE_TO_LAUNCH;
extern Str HWRAPPER_HEARTBEAT_FAILURE;
extern Str HWRAPPER_EXCEPT_CREATE_DUEL;
extern Str HWRAPPER_EXCEPT_MAX_LOOP_COUNT;
extern Str HWRAPPER_EXCEPT_PROC_CRASHED;
extern Str HWRAPPER_EXCEPT_PROC_UNRESPONSIVE;

extern Str CLIENT_ROOM_HOSTING_INVALID_NAME;
extern Str CLIENT_ROOM_HOSTING_NOT_FOUND;
extern Str CLIENT_ROOM_HOSTING_INVALID_MSG;
extern Str CLIENT_ROOM_HOSTING_KICKED_BEFORE;

extern Str CLIENT_ROOM_MSG_RETRY_ERROR;

extern Str ROOM_DUELING_CORE_EXCEPT_CREATION;
extern Str ROOM_DUELING_CORE_EXCEPT_EXTRA_CARDS;
extern Str ROOM_DUELING_CORE_EXCEPT_STARTING;
extern Str ROOM_DUELING_CORE_EXCEPT_RESPONSE;
extern Str ROOM_DUELING_CORE_EXCEPT_PROCESSING;
extern Str ROOM_DUELING_CORE_EXCEPT_DESTRUCTOR;
extern Str ROOM_DUELING_MSG_RETRY_RECEIVED;
extern Str CLIENT_ROOM_REPLAY_TOO_BIG;
extern Str CLIENT_ROOM_CORE_EXCEPT;

extern Str CLIENT_ROOM_KICKED;

extern Str BANLIST_PROVIDER_LOADING_ONE;
extern Str BANLIST_PROVIDER_COULD_NOT_LOAD_ONE;

extern Str CORE_PROVIDER_COULD_NOT_CREATE_TMP_DIR;
extern Str CORE_PROVIDER_PATH_IS_FILE_NOT_DIR;
extern Str CORE_PROVIDER_WRONG_CORE_TYPE;
extern Str CORE_PROVIDER_CORE_NOT_FOUND_IN_REPO;
extern Str CORE_PROVIDER_COPYING_CORE_FILE;
extern Str CORE_PROVIDER_FAILED_TO_COPY_CORE_FILE;
extern Str CORE_PROVIDER_VERSION_REPORTED;
extern Str CORE_PROVIDER_ERROR_WHILE_TESTING;

extern Str DATA_PROVIDER_LOADING_ONE;
extern Str DATA_PROVIDER_COULD_NOT_MERGE;

extern Str REPLAY_MANAGER_NOT_SAVING_REPLAYS;
extern Str REPLAY_MANAGER_COULD_NOT_CREATE_DIR;
extern Str REPLAY_MANAGER_PATH_IS_FILE_NOT_DIR;
extern Str REPLAY_MANAGER_ERROR_WRITING_INITIAL_ID;
extern Str REPLAY_MANAGER_CURRENT_ID;
extern Str REPLAY_MANAGER_LASTID_SIZE_CORRUPTED;
extern Str REPLAY_MANAGER_UNABLE_TO_SAVE;
extern Str REPLAY_MANAGER_CANNOT_OPEN_LASTID;
extern Str REPLAY_MANAGER_CANNOT_WRITE_ID;

extern Str SCRIPT_PROVIDER_LOADING_FILES;
extern Str SCRIPT_PROVIDER_COULD_NOT_OPEN;
extern Str SCRIPT_PROVIDER_TOTAL_FILES_LOADED;

} // namespace Ignis::Multirole::I18N

#endif // MULTIROLE_I18N_HPP