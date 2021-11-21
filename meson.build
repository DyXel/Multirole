project('multirole', ['c', 'cpp'], default_options : ['cpp_std=c++17', 'b_vscrt=static_from_buildtype'])

static_deps = host_machine.system() == 'windows'

atomic_dep  = meson.get_compiler('cpp').find_library('atomic', required : false)
boost_dep   = dependency('boost', version : '>=1.75', modules : ['filesystem'], static : static_deps)
dl_dep      = meson.get_compiler('cpp').find_library('dl', required : false)
fmt_dep     = dependency('fmt', version : '>=6.0.0', static : static_deps)
libgit2_dep = dependency('libgit2', static : static_deps)
openssl_dep = dependency('openssl', static : static_deps)
rt_dep      = meson.get_compiler('cpp').find_library('rt', required : false)
sqlite3_dep = dependency('sqlite3', static : static_deps)
tcm_dep     = dependency('libtcmalloc', required : get_option('use_tcmalloc'), static : static_deps)
thread_dep  = dependency('threads')

multirole_src_files = files([
	'src/DLOpen.cpp',
	'src/Multirole/GitRepo.cpp',
	'src/Multirole/I18N.cpp',
	'src/Multirole/Instance.cpp',
	'src/Multirole/Lobby.cpp',
	'src/Multirole/main.cpp',
	'src/Multirole/STOCMsgFactory.cpp',
	'src/Multirole/Core/DLWrapper.cpp',
	'src/Multirole/Core/HornetWrapper.cpp',
	'src/Multirole/Endpoint/LobbyListing.cpp',
	'src/Multirole/Endpoint/RoomHosting.cpp',
	'src/Multirole/Endpoint/Webhook.cpp',
	'src/Multirole/Room/Client.cpp',
	'src/Multirole/Room/Context.cpp',
	'src/Multirole/Room/Instance.cpp',
	'src/Multirole/Room/ScriptLogger.cpp',
	'src/Multirole/Room/TimerAggregator.cpp',
	'src/Multirole/Room/State/ChoosingTurn.cpp',
	'src/Multirole/Room/State/Closing.cpp',
	'src/Multirole/Room/State/Dueling.cpp',
	'src/Multirole/Room/State/Rematching.cpp',
	'src/Multirole/Room/State/RockPaperScissor.cpp',
	'src/Multirole/Room/State/Sidedecking.cpp',
	'src/Multirole/Room/State/Waiting.cpp',
	'src/Multirole/Service/BanlistProvider.cpp',
	'src/Multirole/Service/CoreProvider.cpp',
	'src/Multirole/Service/DataProvider.cpp',
	'src/Multirole/Service/LogHandler.cpp',
	'src/Multirole/Service/ReplayManager.cpp',
	'src/Multirole/Service/ScriptProvider.cpp',
	'src/Multirole/Service/LogHandler/DiscordWebhookSink.cpp',
	'src/Multirole/Service/LogHandler/FileSink.cpp',
	'src/Multirole/Service/LogHandler/StderrSink.cpp',
	'src/Multirole/Service/LogHandler/StdoutSink.cpp',
	'src/Multirole/Service/LogHandler/StreamFormat.cpp',
	'src/Multirole/Service/LogHandler/Timestamp.cpp',
	'src/Multirole/YGOPro/Banlist.cpp',
	'src/Multirole/YGOPro/CardDatabase.cpp',
	'src/Multirole/YGOPro/CoreUtils.cpp',
	'src/Multirole/YGOPro/Deck.cpp',
	'src/Multirole/YGOPro/Replay.cpp',
	'src/Multirole/YGOPro/StringUtils.cpp',
	'src/Multirole/YGOPro/LZMA/Alloc.c',
	'src/Multirole/YGOPro/LZMA/LzFind.c',
	'src/Multirole/YGOPro/LZMA/LzmaEnc.c'
])

hornet_src_files = files([
	'src/DLOpen.cpp',
	'src/Hornet/main.cpp'
])

executable('multirole', multirole_src_files,
	c_args: [
		'-D_7ZIP_ST',
		'-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS',
		'-D_WINSOCK_DEPRECATED_NO_WARNINGS',
		'-DNOMINMAX'
	],
	cpp_args: [
		'-DBOOST_DATE_TIME_NO_LIB',
		'-DBOOST_JSON_STANDALONE',
		'-DNOMINMAX'
	],
	dependencies: [
		atomic_dep,
		boost_dep,
		dl_dep,
		fmt_dep,
		libgit2_dep,
		openssl_dep,
		rt_dep,
		sqlite3_dep,
		tcm_dep,
		thread_dep
	])

executable('hornet', hornet_src_files,
	cpp_args: [
		'-DBOOST_DATE_TIME_NO_LIB',
		'-DNOMINMAX'
	],
	dependencies: [
		boost_dep,
		dl_dep,
		rt_dep
	])
