#pragma once

#include "Experiment.h"
#include "options.h"
#include "sqlite3.h"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <cctype>

class ResultDatabase {
private:
	sqlite3* db = nullptr;

	static std::string CurrentTimestamp() {
		const auto now = std::chrono::system_clock::now();
		const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
		std::tm tm_buf{};
#if defined(_WIN32)
		localtime_s(&tm_buf, &now_c);
#else
		localtime_r(&now_c, &tm_buf);
#endif
		std::ostringstream oss;
		oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
		return oss.str();
	}

	std::unordered_set<std::string> GetTableColumns(const std::string& table_name) {
		std::unordered_set<std::string> columns;
		const std::string sql = "PRAGMA table_info(" + table_name + ");";
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
			throw std::runtime_error("Could not inspect columns for table " + table_name + ": " + sqlite3_errmsg(db));
		}
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			const unsigned char* col_name = sqlite3_column_text(stmt, 1);
			if (col_name) {
				columns.insert(reinterpret_cast<const char*>(col_name));
			}
		}
		sqlite3_finalize(stmt);
		return columns;
	}

	void EnsureColumnExists(const std::string& table_name, const std::string& column_name, const std::string& column_def) {
		const auto columns = GetTableColumns(table_name);
		if (columns.find(column_name) == columns.end()) {
			const std::string alter_sql = "ALTER TABLE " + table_name + " ADD COLUMN " + column_name + " " + column_def + ";";
			char* err_msg = nullptr;
			if (sqlite3_exec(db, alter_sql.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK) {
				std::string error = err_msg ? err_msg : sqlite3_errmsg(db);
				sqlite3_free(err_msg);
				throw std::runtime_error("Could not alter table " + table_name + " (" + column_name + "): " + error);
			}
		}
	}

	void InitSchema() {
		const char* create_inputs =
			"CREATE TABLE IF NOT EXISTS inputs ("
			"_id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"filename TEXT,"
			"vertex_count INTEGER,"
			"optimal_result INTEGER"
			");";

		const char* create_experiments =
			"CREATE TABLE IF NOT EXISTS experiments ("
			"_id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"created_at TEXT,"
			"short_name TEXT,"
			"description TEXT"
			");";

		const char* create_results =
			"CREATE TABLE IF NOT EXISTS results ("
			"_id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"input_file INTEGER REFERENCES inputs(_id),"
			"experiment_id INTEGER REFERENCES experiments(_id),"
			"num_colors_result INTEGER,"
			"time_taken REAL,"
			"max_iter_between INTEGER,"
			"max_time_between REAL"
			");";

		char* err_msg = nullptr;
		if (sqlite3_exec(db, create_inputs, nullptr, nullptr, &err_msg) != SQLITE_OK ||
			sqlite3_exec(db, create_experiments, nullptr, nullptr, &err_msg) != SQLITE_OK ||
			sqlite3_exec(db, create_results, nullptr, nullptr, &err_msg) != SQLITE_OK) {
			std::string error = err_msg ? err_msg : sqlite3_errmsg(db);
			sqlite3_free(err_msg);
			throw std::runtime_error("Could not initialize results database: " + error);
		}

		// Migracja starszej bazy: nowe kolumny są NULLABLE, aby zachować stare rekordy.
		EnsureColumnExists("experiments", "created_at", "TEXT");
		EnsureColumnExists("experiments", "short_name", "TEXT");
		EnsureColumnExists("experiments", "description", "TEXT");

		EnsureColumnExists("results", "experiment_id", "INTEGER REFERENCES experiments(_id)");
		EnsureColumnExists("results", "max_iter_between", "INTEGER");
		EnsureColumnExists("results", "max_time_between", "REAL");
	}

public:
	ResultDatabase() {
		if (sqlite3_open(SQLITE_DATABASE_PATH, &db) != SQLITE_OK) {
			if (db)
				sqlite3_close(db);
			db = nullptr;
			throw std::runtime_error("Could not open results database");
		}
		try {
			InitSchema();
		} catch (...) {
			sqlite3_close(db);
			db = nullptr;
			throw;
		}
	}

	virtual ~ResultDatabase() {
		if (db)
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
		return static_cast<int>(sqlite3_last_insert_rowid(db));
	}

	int RegisterOrGetInput(const std::string& filename, int vertex_count, int optimal_result) {
		sqlite3_stmt* stmt = nullptr;
		const std::string basename = std::filesystem::path(filename).filename().string();
		if (sqlite3_prepare_v2(db, "SELECT _id FROM inputs WHERE filename = ? OR filename = ? ORDER BY CASE WHEN filename = ? THEN 0 ELSE 1 END LIMIT 1;", -1, &stmt, nullptr) != SQLITE_OK)
			throw std::runtime_error("Could not prepare problem database lookup");
		sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, basename.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, filename.c_str(), -1, SQLITE_TRANSIENT);
		int step_result = sqlite3_step(stmt);
		if (step_result == SQLITE_ROW) {
			int id = sqlite3_column_int(stmt, 0);
			sqlite3_finalize(stmt);
			if (sqlite3_prepare_v2(db, "UPDATE inputs SET optimal_result = ?, vertex_count = ? WHERE _id = ?;", -1, &stmt, nullptr) != SQLITE_OK)
				throw std::runtime_error("Could not prepare problem metadata update");
			sqlite3_bind_int(stmt, 1, optimal_result);
			sqlite3_bind_int(stmt, 2, vertex_count);
			sqlite3_bind_int(stmt, 3, id);
			if (sqlite3_step(stmt) != SQLITE_DONE) {
				std::string error = sqlite3_errmsg(db);
				sqlite3_finalize(stmt);
				throw std::runtime_error("Could not update problem metadata: " + error);
			}
			sqlite3_finalize(stmt);
			return id;
		}
		sqlite3_finalize(stmt);
		if (step_result != SQLITE_DONE)
			throw std::runtime_error("Could not look up problem in database");
		return RegisterInput(filename, vertex_count, optimal_result);
	}

	int CreateExperiment(const std::string& short_name, const std::string& description) {
		const std::string created_at = CurrentTimestamp();
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, "INSERT INTO experiments (created_at, short_name, description) VALUES (?, ?, ?);", -1, &stmt, nullptr) != SQLITE_OK)
			throw std::runtime_error("Could not prepare experiment insert");
		sqlite3_bind_text(stmt, 1, created_at.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, short_name.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, description.c_str(), -1, SQLITE_TRANSIENT);
		if (sqlite3_step(stmt) != SQLITE_DONE) {
			std::string error = sqlite3_errmsg(db);
			sqlite3_finalize(stmt);
			throw std::runtime_error("Could not create experiment: " + error);
		}
		sqlite3_finalize(stmt);
		return static_cast<int>(sqlite3_last_insert_rowid(db));
	}

	std::optional<Experiment> GetExperiment(int experiment_id) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, "SELECT _id, created_at, short_name, description FROM experiments WHERE _id = ?;", -1, &stmt, nullptr) != SQLITE_OK)
			throw std::runtime_error("Could not prepare experiment lookup");
		sqlite3_bind_int(stmt, 1, experiment_id);
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			Experiment exp;
			exp.id = sqlite3_column_int(stmt, 0);
			const unsigned char* c_at = sqlite3_column_text(stmt, 1);
			const unsigned char* c_name = sqlite3_column_text(stmt, 2);
			const unsigned char* c_desc = sqlite3_column_text(stmt, 3);
			exp.created_at = c_at ? reinterpret_cast<const char*>(c_at) : "";
			exp.short_name = c_name ? reinterpret_cast<const char*>(c_name) : "";
			exp.description = c_desc ? reinterpret_cast<const char*>(c_desc) : "";
			sqlite3_finalize(stmt);
			return exp;
		}
		sqlite3_finalize(stmt);
		return std::nullopt;
	}

	std::vector<Experiment> ListExperiments() {
		std::vector<Experiment> list;
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, "SELECT _id, created_at, short_name, description FROM experiments ORDER BY _id DESC;", -1, &stmt, nullptr) != SQLITE_OK)
			throw std::runtime_error("Could not prepare experiments list");
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			Experiment exp;
			exp.id = sqlite3_column_int(stmt, 0);
			const unsigned char* c_at = sqlite3_column_text(stmt, 1);
			const unsigned char* c_name = sqlite3_column_text(stmt, 2);
			const unsigned char* c_desc = sqlite3_column_text(stmt, 3);
			exp.created_at = c_at ? reinterpret_cast<const char*>(c_at) : "";
			exp.short_name = c_name ? reinterpret_cast<const char*>(c_name) : "";
			exp.description = c_desc ? reinterpret_cast<const char*>(c_desc) : "";
			list.push_back(std::move(exp));
		}
		sqlite3_finalize(stmt);
		return list;
	}

	void SaveResult(
		int input_file_id,
		int experiment_id,
		int num_colors,
		double time_taken,
		std::optional<int> max_iter_between = std::nullopt,
		std::optional<double> max_time_between = std::nullopt
	) {
		sqlite3_stmt* stmt = nullptr;
		const char* sql =
			"INSERT INTO results "
			"(input_file, experiment_id, num_colors_result, time_taken, max_iter_between, max_time_between) "
			"VALUES (?, ?, ?, ?, ?, ?);";
		if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
			throw std::runtime_error("Could not prepare result database insert");

		sqlite3_bind_int(stmt, 1, input_file_id);
		sqlite3_bind_int(stmt, 2, experiment_id);
		sqlite3_bind_int(stmt, 3, num_colors);
		sqlite3_bind_double(stmt, 4, time_taken);

		if (max_iter_between.has_value())
			sqlite3_bind_int(stmt, 5, *max_iter_between);
		else
			sqlite3_bind_null(stmt, 5);

		if (max_time_between.has_value())
			sqlite3_bind_double(stmt, 6, *max_time_between);
		else
			sqlite3_bind_null(stmt, 6);

		if (sqlite3_step(stmt) != SQLITE_DONE) {
			std::string error = sqlite3_errmsg(db);
			sqlite3_finalize(stmt);
			throw std::runtime_error("Could not save result: " + error);
		}
		sqlite3_finalize(stmt);
	}

	std::vector<ExperimentResultRow> GetResultsForExperiment(int experiment_id) {
		std::vector<ExperimentResultRow> rows;
		sqlite3_stmt* stmt = nullptr;
		const char* sql =
			"SELECT "
			"  r._id, "
			"  r.input_file, "
			"  r.experiment_id, "
			"  i.filename, "
			"  i.vertex_count, "
			"  i.optimal_result, "
			"  r.num_colors_result, "
			"  r.time_taken, "
			"  r.max_iter_between, "
			"  r.max_time_between "
			"FROM results r "
			"LEFT JOIN inputs i ON r.input_file = i._id "
			"WHERE r.experiment_id = ? "
			"ORDER BY r._id ASC;";
		if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
			throw std::runtime_error("Could not prepare results lookup for experiment");

		sqlite3_bind_int(stmt, 1, experiment_id);
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			ExperimentResultRow row;
			row.id = sqlite3_column_int(stmt, 0);
			row.input_file_id = sqlite3_column_int(stmt, 1);
			row.experiment_id = sqlite3_column_int(stmt, 2);

			const unsigned char* c_filename = sqlite3_column_text(stmt, 3);
			row.filename = c_filename ? reinterpret_cast<const char*>(c_filename) : "";

			if (sqlite3_column_type(stmt, 4) != SQLITE_NULL)
				row.vertex_count = sqlite3_column_int(stmt, 4);

			if (sqlite3_column_type(stmt, 5) != SQLITE_NULL)
				row.optimal_result = sqlite3_column_int(stmt, 5);

			row.num_colors_result = sqlite3_column_int(stmt, 6);
			row.time_taken = sqlite3_column_double(stmt, 7);

			if (sqlite3_column_type(stmt, 8) != SQLITE_NULL)
				row.max_iter_between = sqlite3_column_int(stmt, 8);

			if (sqlite3_column_type(stmt, 9) != SQLITE_NULL)
				row.max_time_between = sqlite3_column_double(stmt, 9);

			rows.push_back(std::move(row));
		}
		sqlite3_finalize(stmt);
		return rows;
	}

	std::optional<int> ResolveExperimentId(const std::string& id_or_name) {
		if (id_or_name.empty())
			return std::nullopt;

		// Próba sparsowania jako ID (cała wartość musi być liczbą)
		bool all_digits = std::all_of(id_or_name.begin(), id_or_name.end(), [](unsigned char ch) {
			return std::isdigit(ch) != 0;
		});
		if (all_digits) {
			try {
				int id = std::stoi(id_or_name);
				if (GetExperiment(id).has_value())
					return id;
			} catch (...) {
				// ignoruj overflow przy stoi
			}
		}

		// Wyszukanie po dokładnej nazwie (ostatni wpis o tej nazwie)
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, "SELECT _id FROM experiments WHERE short_name = ? ORDER BY _id DESC LIMIT 1;", -1, &stmt, nullptr) != SQLITE_OK)
			throw std::runtime_error("Could not prepare experiment name lookup");
		sqlite3_bind_text(stmt, 1, id_or_name.c_str(), -1, SQLITE_TRANSIENT);
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			int id = sqlite3_column_int(stmt, 0);
			sqlite3_finalize(stmt);
			return id;
		}
		sqlite3_finalize(stmt);
		return std::nullopt;
	}
};

