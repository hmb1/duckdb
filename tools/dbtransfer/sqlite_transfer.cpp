#include "sqlite_transfer.hpp"

#include "common/types/date.hpp"

using namespace duckdb;
using namespace std;

namespace sqlite {

bool TransferDatabase(DuckDBConnection &con, sqlite3 *sqlite) {
	char *error;
	// start the SQLite transaction
	if (sqlite3_exec(sqlite, "BEGIN TRANSACTION", nullptr, nullptr, &error) != SQLITE_OK) {
		return false;
	}

	// query the list of tables
	auto table_list = con.Query("SELECT name, sql FROM sqlite_master();");

	for (size_t i = 0; i < table_list->size(); i++) {
		auto name = string(table_list->GetValue<const char *>(0, i));
		auto sql = table_list->GetValue<const char *>(1, i);

		// for each table, first create the table in sqlite
		if (sqlite3_exec(sqlite, sql, nullptr, nullptr, &error) != SQLITE_OK) {
			return false;
		}

		// now transfer the actual data
		// first get the data from DuckDB
		auto result = con.Query("SELECT * FROM " + name);
		// create the prepared statement based on the result
		stringstream prepared;
		prepared << "INSERT INTO " << name << " (";
		for (size_t j = 0; j < result->column_count(); j++) {
			prepared << result->names[j];
			if (j + 1 != result->column_count()) {
				prepared << ",";
			}
		}
		prepared << ") VALUES (";
		for (size_t j = 0; j < result->column_count(); j++) {
			prepared << "?";
			if (j + 1 != result->column_count()) {
				prepared << ",";
			}
		}
		prepared << ");";

		auto insert_statement = prepared.str();
		sqlite3_stmt *stmt;
		if (sqlite3_prepare_v2(sqlite, insert_statement.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
			return false;
		}

		auto &types = result->collection.types;
		for (size_t k = 0; k < result->size(); k++) {
			int rc = SQLITE_ERROR;
			for (size_t j = 0; j < types.size(); j++) {
				size_t bind_index = j + 1;
				if (result->ValueIsNull(j, k)) {
					rc = sqlite3_bind_null(stmt, bind_index);
				} else {
					// bind based on the type
					switch (types[j]) {
					case TypeId::BOOLEAN:
						rc = sqlite3_bind_int(stmt, bind_index, (int)result->GetValue<bool>(j, k));
						break;
					case TypeId::TINYINT:
						rc = sqlite3_bind_int(stmt, bind_index, (int)result->GetValue<int8_t>(j, k));
						break;
					case TypeId::SMALLINT:
						rc = sqlite3_bind_int(stmt, bind_index, (int)result->GetValue<int16_t>(j, k));
						break;
					case TypeId::INTEGER:
						rc = sqlite3_bind_int(stmt, bind_index, (int)result->GetValue<int32_t>(j, k));
						break;
					case TypeId::BIGINT:
						rc = sqlite3_bind_int64(stmt, bind_index, (sqlite3_int64)result->GetValue<int64_t>(j, k));
						break;
					case TypeId::POINTER:
						rc = sqlite3_bind_int64(stmt, bind_index, (sqlite3_int64)result->GetValue<uint64_t>(j, k));
						break;
					case TypeId::DATE: {
						auto date = result->GetValue<date_t>(j, k);
						auto date_str = Date::ToString(date) + " 00:00:00";

						rc = sqlite3_bind_text(stmt, bind_index, date_str.c_str(), -1, SQLITE_TRANSIENT);
						break;
					}
					case TypeId::TIMESTAMP:
						// TODO
						throw NotImplementedException("Transferring timestamps is not supported yet");
					case TypeId::DECIMAL:
						rc = sqlite3_bind_double(stmt, bind_index, result->GetValue<double>(j, k));
						break;
					case TypeId::VARCHAR:
						rc = sqlite3_bind_text(stmt, bind_index, result->GetValue<const char *>(j, k), -1,
						                       SQLITE_TRANSIENT);
						break;
					default:
						break;
					}
				}
				if (rc != SQLITE_OK) {
					return false;
				}
			}
			rc = sqlite3_step(stmt);
			if (rc != SQLITE_DONE) {
				return false;
			}
			if (sqlite3_reset(stmt) != SQLITE_OK) {
				return false;
			}
		}
		sqlite3_finalize(stmt);
	}
	// commit the SQLite transaction
	if (sqlite3_exec(sqlite, "COMMIT", nullptr, nullptr, &error) != SQLITE_OK) {
		return false;
	}
	return true;
}

unique_ptr<DuckDBResult> QueryDatabase(vector<TypeId> result_types, sqlite3 *sqlite, std::string query,
                                       volatile int &interrupt) {
	// prepare the SQL statement
	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(sqlite, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		return nullptr;
	}
	// construct the result
	auto result = make_unique<DuckDBResult>();
	// figure out the types of the columns
	// construct the types of the result
	int col_count = sqlite3_column_count(stmt);
	for (int i = 0; i < col_count; i++) {
		result->names.push_back(sqlite3_column_name(stmt, i));
	}
	DataChunk result_chunk;
	result_chunk.Initialize(result_types);
	int rc = SQLITE_ERROR;
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW && interrupt == 0) {
		// get the value for each of the columns
		for (int i = 0; i < col_count; i++) {
			if (sqlite3_column_type(stmt, i) == SQLITE_NULL) {
				// NULL value
				result_chunk.data[i].nullmask[result_chunk.data[i].count] = true;
			} else {
				// normal value, convert type
				switch (result_types[i]) {
				case TypeId::BOOLEAN:
					((int8_t *)result_chunk.data[i].data)[result_chunk.data[i].count] =
					    sqlite3_column_int(stmt, i) == 0 ? 0 : 1;
					break;
				case TypeId::TINYINT:
					((int8_t *)result_chunk.data[i].data)[result_chunk.data[i].count] =
					    (int8_t)sqlite3_column_int(stmt, i);
					break;
				case TypeId::SMALLINT:
					((int16_t *)result_chunk.data[i].data)[result_chunk.data[i].count] =
					    (int16_t)sqlite3_column_int(stmt, i);
					break;
				case TypeId::INTEGER:
					((int32_t *)result_chunk.data[i].data)[result_chunk.data[i].count] =
					    (int32_t)sqlite3_column_int(stmt, i);
					break;
				case TypeId::BIGINT:
					((int64_t *)result_chunk.data[i].data)[result_chunk.data[i].count] =
					    (int64_t)sqlite3_column_int64(stmt, i);
					break;
				case TypeId::DECIMAL:
					((double *)result_chunk.data[i].data)[result_chunk.data[i].count] =
					    (double)sqlite3_column_double(stmt, i);
					break;
				case TypeId::VARCHAR: {
					Value result((char *)sqlite3_column_text(stmt, i));
					result_chunk.data[i].count++;
					result_chunk.data[i].SetValue(result_chunk.data[i].count - 1, result);
					result_chunk.data[i].count--;
					break;
				}
				case TypeId::DATE: {
					auto unix_time = sqlite3_column_int64(stmt, i);
					((date_t *)result_chunk.data[i].data)[result_chunk.data[i].count] = Date::EpochToDate(unix_time);
					break;
				}
				default:
					throw NotImplementedException("Unimplemented type for SQLite -> DuckDB type conversion");
				}
			}
			result_chunk.data[i].count++;
		}
		if (result_chunk.size() == STANDARD_VECTOR_SIZE) {
			// chunk is filled
			// flush the chunk to the result
			result->collection.Append(result_chunk);
			result_chunk.Reset();
		}
	}
	if (rc != SQLITE_DONE) {
		// failed
		return nullptr;
	}
	if (result_chunk.size() > 0) {
		// final append of any leftover data
		result->collection.Append(result_chunk);
		result_chunk.Reset();
	}
	sqlite3_finalize(stmt);
	return result;
}

}; // namespace sqlite