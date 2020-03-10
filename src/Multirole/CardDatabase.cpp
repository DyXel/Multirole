#include "CardDatabase.hpp"

#include <stdexcept> // std::runtime_error
#include <string>

#include <sqlite3.h>

namespace Ignis::Multirole
{

static constexpr const char* DB_SCHEMAS =
R"(
CREATE TABLE "datas" (
	"id"        INTEGER,
	"ot"        INTEGER,
	"alias"     INTEGER,
	"setcode"   INTEGER,
	"type"      INTEGER,
	"atk"       INTEGER,
	"def"       INTEGER,
	"level"     INTEGER,
	"race"      INTEGER,
	"attribute" INTEGER,
	"category"  INTEGER,
	PRIMARY KEY("id")
);
CREATE TABLE "texts" (
	"id"    INTEGER,
	"name"  TEXT,
	"desc"  TEXT,
	"str1"  TEXT,
	"str2"  TEXT,
	"str3"  TEXT,
	"str4"  TEXT,
	"str5"  TEXT,
	"str6"  TEXT,
	"str7"  TEXT,
	"str8"  TEXT,
	"str9"  TEXT,
	"str10" TEXT,
	"str11" TEXT,
	"str12" TEXT,
	"str13" TEXT,
	"str14" TEXT,
	"str15" TEXT,
	"str16" TEXT,
	PRIMARY KEY("id")
);
)";

static constexpr const char* ATTACH_STMT =
R"(
ATTACH ? AS toMerge;
)";

static constexpr const char* MERGE_DATAS_STMT =
R"(
INSERT OR REPLACE INTO datas SELECT * FROM toMerge.datas;
)";

static constexpr const char* MERGE_TEXTS_STMT =
R"(
INSERT OR REPLACE INTO texts SELECT * FROM toMerge.texts;
)";

static constexpr const char* DETACH_STMT =
R"(
DETACH toMerge;
)";

CardDatabase::CardDatabase() : CardDatabase(":memory:")
{}

CardDatabase::CardDatabase(std::string_view absFilePath)
{
	// Create database
	if(sqlite3_open(absFilePath.data(), &db) != SQLITE_OK)
		throw std::runtime_error(sqlite3_errmsg(db));
	// Prepare database
	char* err = nullptr;
	if(sqlite3_exec(db, DB_SCHEMAS, nullptr, nullptr, &err) == SQLITE_ABORT)
	{
		std::string errStr(err);
		sqlite3_free(err);
		sqlite3_close(db);
		throw std::runtime_error(errStr);
	}
	// Prepare attach statement
	if(sqlite3_prepare_v2(db, ATTACH_STMT, -1, &aStmt, nullptr) != SQLITE_OK)
	{
		std::string errStr(sqlite3_errmsg(db));
		sqlite3_close(db);
		throw std::runtime_error(errStr);
	}
}

CardDatabase::~CardDatabase()
{
	sqlite3_finalize(aStmt);
	sqlite3_close(db);
}

bool CardDatabase::Merge(std::string_view absFilePath)
{
	sqlite3_reset(aStmt);
	sqlite3_bind_text(aStmt, 1, absFilePath.data(), -1, SQLITE_TRANSIENT);
	if(sqlite3_step(aStmt) != SQLITE_DONE)
		return false;
	sqlite3_exec(db, MERGE_DATAS_STMT, nullptr, nullptr, nullptr);
	sqlite3_exec(db, MERGE_TEXTS_STMT, nullptr, nullptr, nullptr);
	sqlite3_exec(db, DETACH_STMT, nullptr, nullptr, nullptr);
	return true;
}

} // namespace Ignis::Multirole
