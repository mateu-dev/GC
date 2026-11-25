#pragma once

#include "options.h"
#include "sqlite3.h"
#include <string>
#include <stdexcept>

class ResultDatabase
{
private:
	sqlite3* db = nullptr;
public:
	ResultDatabase() {
		if (sqlite3_open(SQLITE_DATABASE_PATH, &db) != SQLITE_OK) {
			if (db)
				sqlite3_close(db);
			db = nullptr;
			throw std::runtime_error("Could not open results database");
		}
		if (sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS inputs (_id INTEGER PRIMARY KEY AUTOINCREMENT, filename TEXT, vertex_count INTEGER, optimal_result INTEGER);", nullptr, nullptr, nullptr) != SQLITE_OK ||
			sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS results (_id INTEGER PRIMARY KEY AUTOINCREMENT, input_file INTEGER REFERENCES inputs(_id), num_colors_result INTEGER, time_taken REAL);", nullptr, nullptr, nullptr) != SQLITE_OK) {
			std::string error = sqlite3_errmsg(db);
			sqlite3_close(db);
			db = nullptr;
			throw std::runtime_error("Could not initialize results database: " + error);
		}
	}
	virtual ~ResultDatabase() {
		sqlite3_close(db);
	}

	int RegisterInput(const std::string& filename, int vertex_count, int optimal_result) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, "INSERT INTO inputs (filename, vertex_count, optimal_result) VALUES (?, ?, ?);", -1, &stmt, nullptr) != SQLITE_OK)
			throw std::runtime_error("Could not prepare problem database insert");
		sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_int(stmt, 2, vertex_count);
		sqlite3_bind_int(stmt, 3, optimal_result);
		if (sqlite3_step(stmt) != SQLITE_DONE) {
			std::string error = sqlite3_errmsg(db);
			sqlite3_finalize(stmt);
			throw std::runtime_error("Could not register problem: " + error);
		}
		sqlite3_finalize(stmt);
		return sqlite3_last_insert_rowid(db);
	}

	int RegisterOrGetInput(const std::string& filename, int vertex_count, int optimal_result) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, "SELECT _id FROM inputs WHERE filename = ?;", -1, &stmt, nullptr) != SQLITE_OK)
			throw std::runtime_error("Could not prepare problem database lookup");
		sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_TRANSIENT);
		int step_result = sqlite3_step(stmt);
		if (step_result == SQLITE_ROW) {
			int id = sqlite3_column_int(stmt, 0);
			sqlite3_finalize(stmt);
			return id;
		}
		sqlite3_finalize(stmt);
		if (step_result != SQLITE_DONE)
			throw std::runtime_error("Could not look up problem in database");
		return RegisterInput(filename, vertex_count, optimal_result);
	}

	void SaveResult(int input_file_id, int num_colors, double time_taken) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, "INSERT INTO results (input_file, num_colors_result, time_taken) VALUES (?, ?, ?);", -1, &stmt, nullptr) != SQLITE_OK)
			throw std::runtime_error("Could not prepare result database insert");
		sqlite3_bind_int(stmt, 1, input_file_id);
		sqlite3_bind_int(stmt, 2, num_colors);
		sqlite3_bind_double(stmt, 3, time_taken);
		if (sqlite3_step(stmt) != SQLITE_DONE) {
			std::string error = sqlite3_errmsg(db);
			sqlite3_finalize(stmt);
			throw std::runtime_error("Could not save result: " + error);
		}
		sqlite3_finalize(stmt);
	}
};

