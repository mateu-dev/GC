#pragma once

#include "options.h"
#include "sqlite3.h"

class ResultDatabase
{
private:
	sqlite3* db;
public:
	ResultDatabase() {
		sqlite3_open(SQLITE_DATABASE_PATH, &db);
		sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS inputs (_id INTEGER PRIMARY KEY AUTOINCREMENT, filename TEXT, vertex_count INTEGER, optimal_result INTEGER);", nullptr, nullptr, nullptr);
		sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS results (_id INTEGER PRIMARY KEY AUTOINCREMENT, input_file INTEGER FOREIGN KEY REFERENCES inputs(_id), num_colors_result INTEGER, time_taken REAL);", nullptr, nullptr, nullptr);
	}
	virtual ~ResultDatabase() {
		sqlite3_close(db);
	}

	int RegisterInput(const std::string& filename, int vertex_count, int optimal_result) {
		std::string sql = "INSERT INTO inputs (filename, vertex_count, optimal_result) VALUES ('" + filename + "', " + std::to_string(vertex_count) + ", " + std::to_string(optimal_result) + ");";
		sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
		return sqlite3_last_insert_rowid(db);
	}

	int RegisterOrGetInput(const std::string& filename, int vertex_count, int optimal_result) {
		std::string sql = "SELECT _id FROM inputs WHERE filename = '" + filename + "';";
		sqlite3_stmt* stmt;
		if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				int id = sqlite3_column_int(stmt, 0);
				sqlite3_finalize(stmt);
				return id;
			}
		}
		sqlite3_finalize(stmt);
		return RegisterInput(filename, vertex_count, optimal_result);
	}

	void SaveResult(int input_file_id, int num_colors, double time_taken) {
		std::string sql = "INSERT INTO results (input_file, num_colors_result, time_taken) VALUES (" + std::to_string(input_file_id) + ", " + std::to_string(num_colors) + ", " + std::to_string(time_taken) + ");";
		sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
	}
};

