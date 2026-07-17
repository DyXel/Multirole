#include "CardDatabase.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept> // std::runtime_error
#include <string>

#include <sqlite3.h>

#include "Constants.hpp"

namespace YGOPro
{

namespace
{

constexpr const char* DB_SCHEMAS =
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

constexpr const char* ATTACH_STMT =
R"(
ATTACH ? AS toMerge;
)";

constexpr const char* MERGE_DATAS_STMT =
R"(
INSERT OR REPLACE INTO datas SELECT * FROM toMerge.datas;
)";

constexpr const char* MERGE_TEXTS_STMT =
R"(
INSERT OR REPLACE INTO texts SELECT * FROM toMerge.texts;
)";

constexpr const char* DETACH_STMT =
R"(
DETACH toMerge;
)";

constexpr const char* SEARCH_STMT =
R"(
SELECT id,alias,setcode,type,atk,def,level,race,attribute
FROM datas WHERE datas.id = ?;
)";

constexpr const char* SEARCH2_STMT =
R"(
SELECT ot,category
FROM datas WHERE datas.id = ?;
)";

class OpsToSqlQueryEmitter
{
public:
	OpsToSqlQueryEmitter(uint64_t const* ops, std::string& stmt) noexcept :
		ops(ops), stmt(&stmt) {}

	int Parse(int opsSize) noexcept
	{
		auto offset = stmt->size();
		int r = Visit(opsSize - 1);
		if(r < 0)
			return r;
		if(!allowTokens)
			Emit("((datas.type&0x4000)==0)AND");
		if(!allowAliases)
			Emit("(datas.alias!=0)AND");
		std::reverse(std::next(stmt->begin(), offset), stmt->end());
		return r;
	}

private:
	uint64_t const* ops;
	std::string* stmt;
	bool allowAliases = false;
	bool allowTokens = false;

	int Visit(int top) noexcept
	{
		auto check = [&](int v) -> bool { return top - v >= 0; };
		auto const op = ops[top];
		switch(op)
		{
#define DESCENT() if(top = Visit(top - 1); top < 0) break
#define NULLARY_VAL(opcode, val) \
	case opcode: \
	{ \
		Emit("(datas." #val ")"); \
		break; \
	}
#define UNARY_OP(opcode, optor) \
	case opcode: \
	{ \
		if(!check(1)) \
			return -3; \
		Emit(")"); \
		DESCENT(); \
		Emit("(" #optor); \
		break; \
	}
#define UNARY_VAL_PRED(opcode, val, pred) \
	case opcode: \
	{ \
		if(!check(1)) \
			return -4; \
		Emit(")"); \
		DESCENT(); \
		Emit("(datas." #val #pred); \
		break; \
	}
#define BINARY_OP(opcode, optor) \
	case opcode: \
	{ \
		if(!check(2)) \
			return -5; \
		Emit(")"); \
		DESCENT(); \
		Emit(#optor); \
		DESCENT(); \
		Emit("("); \
		break; \
	}
		NULLARY_VAL(OPCODE_GETCODE, id);
		NULLARY_VAL(OPCODE_GETTYPE, type);
		NULLARY_VAL(OPCODE_GETRACE, race);
		NULLARY_VAL(OPCODE_GETATTRIBUTE, attribute);
		UNARY_OP(OPCODE_NEG, -);
		UNARY_OP(OPCODE_NOT, NOT);
		UNARY_OP(OPCODE_BNOT, ~);
		UNARY_VAL_PRED(OPCODE_ISCODE, id, ==);
		UNARY_VAL_PRED(OPCODE_ISTYPE, type, &);
		UNARY_VAL_PRED(OPCODE_ISRACE, race, &);
		UNARY_VAL_PRED(OPCODE_ISATTRIBUTE, attribute, &);
		BINARY_OP(OPCODE_ADD, +);
		BINARY_OP(OPCODE_SUB, -);
		BINARY_OP(OPCODE_MUL, *);
		BINARY_OP(OPCODE_DIV, /);
		BINARY_OP(OPCODE_AND, AND);
		BINARY_OP(OPCODE_OR, OR);
		BINARY_OP(OPCODE_BAND, &);
		BINARY_OP(OPCODE_BOR, |);
		BINARY_OP(OPCODE_LSHIFT, <<);
		BINARY_OP(OPCODE_RSHIFT, >>);
#undef BINARY_OP
#undef UNARY_OP
#undef UNARY_VAL_PRED
#undef NULLARY_VAL
		// Special case: SQLite does not have a binary XOR operator
		case OPCODE_BXOR:
			if(!check(2))
				return -6;
			Emit("))");
			DESCENT();
			Emit(",");
			DESCENT();
			Emit("(ocg_bxor(");
			break;
		// Special case: could be multiple values packed together OR
		// a blob, so we need a named function added before-hand with
		// sqlite3_create_function
		case OPCODE_ISSETCARD:
		{
			if(!check(1))
				return -7;
			Emit(",datas.setcode))");
			DESCENT();
			Emit("(ocg_is_set(");
			break;
		}
		// Special cases: These set a state, the actual expression
		// is appended after the initial pass
		case OPCODE_ALLOW_ALIASES:
		{
			allowAliases = true;
			if(check(1))
				DESCENT();
			break;
		}
		case OPCODE_ALLOW_TOKENS:
		{
			allowTokens = true;
			if(check(1))
				DESCENT();
			break;
		}
		default:
		{
			Emit(")");
			Emit(std::to_string(op));
			Emit("(");
			break;
		}
		}
		return top;
#undef DESCENT
	}

	auto Emit(std::string_view s) -> void
	{
		// append the string reversed so that the allocations are done to the right, avoiding moving the memory
		stmt->append(s.rbegin(), s.rend());
	}
};

void sqlOcgIsSet(sqlite3_context *context, int argc, sqlite3_value **argv)
{
	auto const setTuple = [&]()
	{
		auto const setCode = static_cast<uint16_t>(sqlite3_value_int64(argv[0]));
		return std::tuple(setCode & 0x0FFF, setCode & 0xF000);
	}();
	auto Check = [&](uint16_t setCode) -> bool
	{
		auto const [setType, setSubtype] = setTuple;
		return (setCode & 0x0FFF) == setType &&
		       (setCode & 0xF000 & setSubtype) == setSubtype;
	};
	int match = 0;
	if(int const t = sqlite3_value_type(argv[1]); t == SQLITE_INTEGER)
	{
		auto const setCodes = static_cast<uint64_t>(sqlite3_value_int64(argv[1]));
		auto Demux = [&](uint16_t i) -> uint16_t { return (setCodes >> (i * 16)) & 0xFFFF; };
		match = Check(Demux(0)) | Check(Demux(1)) | Check(Demux(2)) | Check(Demux(3));
	}
	else if(t == SQLITE_BLOB)
	{
		auto const size = static_cast<size_t>(sqlite3_value_bytes(argv[1]));
		auto const* data = static_cast<uint8_t const*>(sqlite3_value_blob(argv[1]));
		for(size_t i = size / 2; i < size; i++, data += sizeof(uint16_t))
		{
			uint16_t setCode;
			std::memcpy(&setCode, data, sizeof(setCode));
			if((match = Check(setCode)))
				break;
		}
	}
	sqlite3_result_int(context, match);
}

void sqlOcgBxor(sqlite3_context *context, int argc, sqlite3_value **argv)
{
	auto result = static_cast<uint64_t>(sqlite3_value_int64(argv[0])) ^
				  static_cast<uint64_t>(sqlite3_value_int64(argv[1]));
	sqlite3_result_int64(context, static_cast<sqlite3_int64>(result));
}

std::array constexpr ocgOpcodeSqliteFuncs
{
	std::pair{"ocg_is_set", &sqlOcgIsSet},
	std::pair{"ocg_bxor", &sqlOcgBxor},
};

} // namespace

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
	// Add function(s) for opcode-based search (see OpsToSqlQueryEmitter)
	for(auto const& [sqliteFuncName, sqliteFuncPtr] : ocgOpcodeSqliteFuncs)
	{
		if(sqlite3_create_function(db, sqliteFuncName, 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
		   nullptr, sqliteFuncPtr, nullptr, nullptr) != SQLITE_OK)
		{
			std::string errStr(sqlite3_errmsg(db));
			sqlite3_close(db);
			throw std::runtime_error(errStr);
		}
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
		cd.race = sqlite3_column_int64(sStmt, 7);
		cd.attribute = sqlite3_column_int(sStmt, 8);
	}
	return cd;
}

void CardDatabase::DataUsageDone([[maybe_unused]] const OCG_CardData& data) const noexcept
{
	// We could remove the elements here, but then what would be the
	// the point of the cache?
}

int CardDatabase::CountDeclarableCards(uint64_t const* ops, int opsSize) const noexcept
{
	if(ops == nullptr)
		return -1;
	if(opsSize <= 0)
		return -2;
	std::string stmtStr = "SELECT COUNT(1) FROM datas WHERE\n";
	if(int r = OpsToSqlQueryEmitter{ops, stmtStr}.Parse(opsSize); r < 0)
		return r;
	std::scoped_lock lock(mDb);
	sqlite3_stmt* stmt{};
	int r = sqlite3_prepare_v2(db, stmtStr.c_str(), -1, &stmt, nullptr);
	if(r != SQLITE_OK)
		return -8;
	if(stmt == nullptr)
		return -9;
	int count = 0;
	if(sqlite3_step(stmt) == SQLITE_ROW)
		count = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);
	return count;
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
