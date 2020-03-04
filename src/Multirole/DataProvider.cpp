#include "DataProvider.hpp"

#include <cstring> // std::memset
#include <stdexcept> // std::runtime_error

#include <sqlite3.h>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

constexpr int TYPE_LINK = 0x4000000; // NOTE: remove if we import other types

namespace Ignis
{

namespace Multirole
{

static constexpr const char* SEARCH_STMT =
R"(
SELECT id,alias,setcode,type,atk,def,level,race,attribute
FROM datas WHERE datas.id = ?;
)";

// public

DataProvider::DataProvider(std::string_view fnRegexStr) :
	CardDatabase(),
	fnRegex(fnRegexStr.data())
{
	// Prepare card data search by id statement
	if(sqlite3_prepare_v2(db, SEARCH_STMT, -1, &sStmt, nullptr) != SQLITE_OK)
		throw std::runtime_error(sqlite3_errmsg(db));
}

DataProvider::~DataProvider()
{
	sqlite3_finalize(sStmt);
}

void DataProvider::OnAdd(std::string_view path, const PathVector& fullFileList)
{
	LoadDBs(path, fullFileList);
}

void DataProvider::OnReset(std::string_view path, const PathVector& deltaFileList)
{
	LoadDBs(path, deltaFileList);
}

OCG_CardData DataProvider::DataFromCode(uint32_t code)
{
	auto AllocSetcodes = [](uint64_t dbVal) -> uint16_t*
	{
		static constexpr std::size_t DB_TOTAL_SETCODES = 4;
		uint16_t* setcodes = new uint16_t[DB_TOTAL_SETCODES + 1];
		for(std::size_t i = 0; i < DB_TOTAL_SETCODES; i++)
			setcodes[i] = (dbVal >> (i * 16)) & 0xFFFF;
		setcodes[DB_TOTAL_SETCODES] = 0;
		return setcodes;
	};
	OCG_CardData cd;
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
		cd.link_marker = (cd.type & TYPE_LINK) ? cd.defense : 0;
		cd.defense = (cd.type & TYPE_LINK) ? 0 : cd.defense;
		const auto dbLevel = sqlite3_column_int(sStmt, 6);
		cd.level = dbLevel & 0x800000FF;
		cd.lscale = (dbLevel >> 24) & 0xFF;
		cd.rscale = (dbLevel >> 16) & 0xFF;
		cd.race = sqlite3_column_int(sStmt, 7);
		cd.attribute = sqlite3_column_int(sStmt, 8);
	}
	else
	{
		std::memset(&cd, 0, sizeof(decltype(cd)));
	}
	return cd;
}

void DataProvider::DataUsageDone(OCG_CardData& data)
{
	delete[] data.setcodes;
}

// private

void DataProvider::LoadDBs(std::string_view path, const PathVector& fileList)
{
	std::string fullPath(path);
	for(const auto& fn : fileList)
	{
		if(!std::regex_match(fn, fnRegex))
			continue;
		fullPath.resize(path.size());
		fullPath += fn;
		spdlog::info(FMT_STRING("DataProvider: Loading up {:s}..."), fullPath);
		if(!Merge(fullPath))
		{
			spdlog::error("DataProvider: Couldn't merge database");
		}
	}
}

} // namespace Multirole

} // namespace Ignis
