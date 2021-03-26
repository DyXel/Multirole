constexpr std::array<
	const char* const,
	static_cast<std::size_t>(ServiceType::SERVICE_TYPE_COUNT)
> SVC_NAMES =
{
	"GitRepository",
	"Multirole",
	"BanlistProvider",
	"CoreProvider",
	"DataProvider",
	"LogHandler",
	"ReplayManager",
	"ScriptProvider",
};
