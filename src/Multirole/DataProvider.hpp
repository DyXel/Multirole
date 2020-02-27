#ifndef DATAPROVIDER_HPP
#define DATAPROVIDER_HPP
#include <regex>

#include "CardDatabase.hpp"
#include "IGitRepoObserver.hpp"
#include "Core/IDataSupplier.hpp"

namespace Ignis
{

namespace Multirole
{

class IAsyncLogger;

class DataProvider final : public CardDatabase, public IGitRepoObserver,
                           public Core::IDataSupplier
{
public:
	DataProvider(IAsyncLogger& l, std::string_view fnRegexStr);
	~DataProvider();

	// IGitRepoObserver overrides
	void OnAdd(std::string_view path, const PathVector& fullFileList) override;
	void OnReset(std::string_view path, const PathVector& deltaFileList) override;

	// Core::IDataSupplier overrides
	OCG_CardData DataFromCode(uint32_t code) override;
	void DataUsageDone(OCG_CardData& data) override;
private:
	IAsyncLogger& logger;
	std::regex fnRegex;
	sqlite3_stmt* sStmt;

	void LoadDBs(std::string_view path, const PathVector& fullFileList);
};

} // namespace Multirole

} // namespace Ignis

#endif // DATAPROVIDER_HPP
