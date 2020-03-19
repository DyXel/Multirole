#ifndef CARDDATABASE_HPP
#define CARDDATABASE_HPP
#include <string_view>
#include "Core/IDataSupplier.hpp"

struct sqlite3;
struct sqlite3_stmt;

namespace Ignis::Multirole
{

class CardDatabase : public Core::IDataSupplier
{
public:
	// Creates a in-memory database
	CardDatabase();

	// Closes the database
	~CardDatabase();

	// Opens or creates a disk database
	CardDatabase(std::string_view absFilePath);

	// Add a new database to the amalgamation
	bool Merge(std::string_view absFilePath);

	// Core::IDataSupplier overrides
	OCG_CardData DataFromCode(uint32_t code) override;
	void DataUsageDone(OCG_CardData& data) override;
protected:
	sqlite3* db{};
private:
	sqlite3_stmt* aStmt{};
	sqlite3_stmt* sStmt{};
};

} // namespace Ignis::Multirole

#endif // CARDDATABASE_HPP
