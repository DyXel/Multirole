#ifndef DATAPROVIDER_HPP
#define DATAPROVIDER_HPP
#include <regex>

#include "CardDatabase.hpp"
#include "IGitRepoObserver.hpp"
#include "Core/IDataSupplier.hpp"

namespace Ignis::Multirole
{

class DataProvider final : public CardDatabase, public IGitRepoObserver,
                           public Core::IDataSupplier
{
public:
	DataProvider(std::string_view fnRegexStr);
	~DataProvider();

	// IGitRepoObserver overrides
	void OnAdd(std::string_view path, const PathVector& fileList) override;
	void OnDiff(std::string_view path, const GitDiff& diff) override;

	// Core::IDataSupplier overrides
	OCG_CardData DataFromCode(uint32_t code) override;
	void DataUsageDone(OCG_CardData& data) override;
private:
	std::regex fnRegex;
	sqlite3_stmt* sStmt{};

	void LoadDBs(std::string_view path, const PathVector& fullFileList);
};

} // namespace Ignis::Multirole

#endif // DATAPROVIDER_HPP
