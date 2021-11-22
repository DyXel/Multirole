#ifndef CARDDATABASE_HPP
#define CARDDATABASE_HPP
#include <string_view>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "../Core/IDataSupplier.hpp"

struct sqlite3;
struct sqlite3_stmt;

namespace YGOPro
{

struct CardExtraData
{
	uint32_t scope;
	uint32_t category;
};

class CardDatabase final : public Ignis::Multirole::Core::IDataSupplier
{
public:
	// Creates a in-memory database
	CardDatabase();

	// Closes the database
	~CardDatabase() noexcept;

	// Opens or creates a disk database
	CardDatabase(std::string_view absFilePath);

	// Add a new database to the amalgamation
	bool Merge(std::string_view absFilePath) noexcept;

	// Core::IDataSupplier overrides
	const OCG_CardData& DataFromCode(uint32_t code) const noexcept override;
	void DataUsageDone(const OCG_CardData& data) const noexcept override;

	// Query extra data
	const CardExtraData& ExtraFromCode(uint32_t code) const noexcept;
private:
	sqlite3* db{};
	sqlite3_stmt* aStmt{};
	sqlite3_stmt* sStmt{};
	sqlite3_stmt* s2Stmt{};

	mutable std::mutex mDb;

	mutable std::unordered_map<uint32_t, OCG_CardData> dataCache;
	mutable std::unordered_map<uint32_t, std::unique_ptr<uint16_t[]>> scCache;
	mutable std::mutex mDataCache;

	mutable std::unordered_map<uint32_t, CardExtraData> extraCache;
	mutable std::mutex mExtraCache;
};

} // namespace YGOPro

#endif // CARDDATABASE_HPP
