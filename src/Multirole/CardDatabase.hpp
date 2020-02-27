#ifndef CARDDATABASE_HPP
#define CARDDATABASE_HPP
#include <string_view>

struct sqlite3;
struct sqlite3_stmt;

namespace Ignis
{

namespace Multirole
{

class CardDatabase
{
public:
	// Creates a in-memory database
	CardDatabase();

	// Opens or creates a disk database
	CardDatabase(std::string_view absFilePath);

	// Closes the database
	~CardDatabase();

	// Add a new database to the amalgation
	bool Merge(std::string_view absFilePath);
protected:
	sqlite3* db;
private:
	sqlite3_stmt* aStmt;
};

} // namespace Multirole

} // namespace Ignis

#endif // CARDDATABASE_HPP
