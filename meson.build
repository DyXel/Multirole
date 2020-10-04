project('multirole', ['c', 'cpp'], default_options : 'cpp_std=c++17')

asio_dep    = dependency('asio', required : host_machine.system() == 'windows')
atomic_dep  = meson.get_compiler('cpp').find_library('atomic', required : false)
dl_dep      = meson.get_compiler('cpp').find_library('dl', required : false)
libgit2_dep = dependency('libgit2')
njson_dep   = dependency('nlohmann_json')
spdlog_dep  = dependency('spdlog')
sqlite3_dep = dependency('sqlite3')
thread_dep  = dependency('threads')

src_files = files([
	'src/DLOpen.cpp',
	'src/Multirole/BanlistProvider.cpp',
	'src/Multirole/CardDatabase.cpp',
	'src/Multirole/CoreProvider.cpp',
	'src/Multirole/DataProvider.cpp',
	'src/Multirole/GitRepo.cpp',
	'src/Multirole/Instance.cpp',
	'src/Multirole/Lobby.cpp',
	'src/Multirole/main.cpp',
	'src/Multirole/ReplayManager.cpp',
	'src/Multirole/ScriptProvider.cpp',
	'src/Multirole/STOCMsgFactory.cpp',
	'src/Multirole/Core/DLWrapper.cpp',
	'src/Multirole/Core/HornetWrapper.cpp',
	'src/Multirole/Endpoint/LobbyListing.cpp',
	'src/Multirole/Endpoint/RoomHosting.cpp',
	'src/Multirole/Endpoint/Webhook.cpp',
	'src/Multirole/Room/Client.cpp',
	'src/Multirole/Room/Context.cpp',
	'src/Multirole/Room/Instance.cpp',
	'src/Multirole/Room/TimerAggregator.cpp',
	'src/Multirole/Room/State/ChoosingTurn.cpp',
	'src/Multirole/Room/State/Closing.cpp',
	'src/Multirole/Room/State/Dueling.cpp',
	'src/Multirole/Room/State/Rematching.cpp',
	'src/Multirole/Room/State/RockPaperScissor.cpp',
	'src/Multirole/Room/State/Sidedecking.cpp',
	'src/Multirole/Room/State/Waiting.cpp',
	'src/Multirole/YGOPro/Banlist.cpp',
	'src/Multirole/YGOPro/CoreUtils.cpp',
	'src/Multirole/YGOPro/Deck.cpp',
	'src/Multirole/YGOPro/Replay.cpp',
	'src/Multirole/YGOPro/StringUtils.cpp',
	'src/Multirole/YGOPro/LZMA/Alloc.c',
	'src/Multirole/YGOPro/LZMA/LzFind.c',
	'src/Multirole/YGOPro/LZMA/LzmaEnc.c'
])

executable('multirole', src_files,
	c_args: ['-D_7ZIP_ST'],
	cpp_args: [
		'-DASIO_STANDALONE',
		'-DSPDLOG_FMT_EXTERNAL'
	],
	dependencies: [
		asio_dep,
		atomic_dep,
		dl_dep,
		libgit2_dep,
		njson_dep,
		spdlog_dep,
		sqlite3_dep,
		thread_dep
	])
