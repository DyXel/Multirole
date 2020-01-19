#ifndef CARDDATABASE_HPP
#define CARDDATABASE_HPP
#include <string_view>
#include <unordered_map>

#include "ocgapi_types.h"

struct sqlite3;
struct sqlite3_stmt;

namespace Placeholder4
{

class CardDatabase
{
public:
	// Creates a in-memory database
	CardDatabase();

	// Opens or creates a disk database
	CardDatabase(std::string_view absFilePath);
	~CardDatabase();

	// Add a new database to the amalgation
	bool Merge(std::string_view absFilePath);

	// Retrieve card ready to be used by the core API
	OCG_CardData CardDataFromCode(unsigned int code, bool& found);
protected:
	sqlite3* db{nullptr};
private:
	sqlite3_stmt* aStmt{nullptr};
	sqlite3_stmt* sStmt{nullptr};
};

} // namespace Placeholder4

#endif // CARDDATABASE_HPP
