#include "CardDatabase.hpp"

#include <cstring>
#include <stdexcept> // std::runtime_error
#include <string>

#include <sqlite3.h>

#include "Constants.hpp"

namespace YGOPro
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

static constexpr const char* SEARCH_STMT =
R"(
SELECT id,alias,setcode,type,atk,def,level,race,attribute
FROM datas WHERE datas.id = ?;
)";

static constexpr const char* SEARCH2_STMT =
R"(
SELECT ot,category
FROM datas WHERE datas.id = ?;
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
	// Prepare card data search by id statement
	if(sqlite3_prepare_v2(db, SEARCH_STMT, -1, &sStmt, nullptr) != SQLITE_OK)
	{
		std::string errStr(sqlite3_errmsg(db));
		sqlite3_finalize(aStmt);
		sqlite3_close(db);
		throw std::runtime_error(errStr);
	}
	// Prepare extra data search by id statement
	if(sqlite3_prepare_v2(db, SEARCH2_STMT, -1, &s2Stmt, nullptr) != SQLITE_OK)
	{
		std::string errStr(sqlite3_errmsg(db));
		sqlite3_finalize(sStmt);
		sqlite3_finalize(aStmt);
		sqlite3_close(db);
		throw std::runtime_error(errStr);
	}
}

CardDatabase::~CardDatabase() noexcept
{
	sqlite3_finalize(s2Stmt);
	sqlite3_finalize(sStmt);
	sqlite3_finalize(aStmt);
	sqlite3_close(db);
}

bool CardDatabase::Merge(std::string_view absFilePath) noexcept
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

const OCG_CardData& CardDatabase::DataFromCode(uint32_t code) const noexcept
{
	std::scoped_lock lock(mDataCache);
	if(auto search = dataCache.find(code); search != dataCache.end())
		return search->second;
	std::scoped_lock lock2(mDb);
	auto AllocSetcodes = [&](uint64_t dbVal) -> uint16_t*
	{
		static constexpr std::size_t SETCODES = 4U;
		auto p = decltype(scCache)::value_type(code, std::make_unique<uint16_t[]>(SETCODES + 1U));
		auto& setcodes = scCache.emplace(std::move(p)).first->second;
		for(std::size_t i = 0U; i < SETCODES; i++)
			setcodes[i] = (dbVal >> (i * 16U)) & 0xFFFF;
		setcodes[SETCODES] = 0U;
		return setcodes.get();
	};
	auto& cd = dataCache.emplace(code, OCG_CardData{}).first->second;
	sqlite3_reset(sStmt);
	sqlite3_bind_int(sStmt, 1, code);
	if(sqlite3_step(sStmt) == SQLITE_ROW)
	{
		cd.code = sqlite3_column_int(sStmt, 0);
		cd.alias = sqlite3_column_int(sStmt, 1);
		cd.setcodes = AllocSetcodes(sqlite3_column_int64(sStmt, 2));
		cd.type = sqlite3_column_int(sStmt, 3);
		cd.attack = sqlite3_column_int(sStmt, 4);
		cd.defense = sqlite3_column_int(sStmt, 5);
		cd.link_marker = (cd.type & TYPE_LINK) != 0U ? cd.defense : 0;
		cd.defense = (cd.type & TYPE_LINK) != 0U ? 0 : cd.defense;
		const auto dbLevel = sqlite3_column_int(sStmt, 6);
		cd.level = dbLevel & 0x800000FF;
		cd.lscale = (dbLevel >> 24U) & 0xFF;
		cd.rscale = (dbLevel >> 16U) & 0xFF;
		cd.race = sqlite3_column_int(sStmt, 7);
		cd.attribute = sqlite3_column_int(sStmt, 8);
	}
	return cd;
}

void CardDatabase::DataUsageDone([[maybe_unused]] const OCG_CardData& data) const noexcept
{
	// We could remove the elements here, but then what would be the
	// the point of the cache?
}

const CardExtraData& CardDatabase::ExtraFromCode(uint32_t code) const noexcept
{
	std::scoped_lock lock(mExtraCache);
	if(auto search = extraCache.find(code); search != extraCache.end())
		return search->second;
	std::scoped_lock lock2(mDb);
	auto& ced = extraCache.emplace(code, CardExtraData{}).first->second;
	sqlite3_reset(s2Stmt);
	sqlite3_bind_int(s2Stmt, 1, code);
	if(sqlite3_step(s2Stmt) == SQLITE_ROW)
	{
		ced.scope = sqlite3_column_int(s2Stmt, 0);
		ced.category = sqlite3_column_int(s2Stmt, 1);
	}
	return ced;
}

} // namespace YGOPro
