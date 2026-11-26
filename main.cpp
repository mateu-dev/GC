#include "json.hpp"
#include "Measure.h"
#include "options.h"
#include "ProblemLoader.h"
#include "ResultDatabase.h"
#include "TabuSearch.h"
#include <algorithm>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string Trim(const std::string& s) {
	const auto first = s.find_first_not_of(" \t\r\n");
	if (first == std::string::npos) return "";
	const auto last = s.find_last_not_of(" \t\r\n");
	return s.substr(first, last - first + 1);
}

std::string PromptLine(const std::string& prompt) {
	std::cout << prompt;
	std::string line;
	std::getline(std::cin, line);
	return Trim(line);
}

void OpenComparisonHtml() {
	const std::string html_path = "./export/view_exp_comparison.html";
#if defined(_WIN32)
	const std::string cmd = "start \"\" \"" + html_path + "\"";
#elif defined(__APPLE__)
	const std::string cmd = "open \"" + html_path + "\"";
#else
	const std::string cmd = "xdg-open \"" + html_path + "\"";
#endif
	std::system(cmd.c_str());
}

nlohmann::json ExperimentToJson(ResultDatabase& db, int experiment_id) {
	auto exp_opt = db.GetExperiment(experiment_id);
	if (!exp_opt.has_value()) {
		throw std::runtime_error("Experiment not found: ID " + std::to_string(experiment_id));
	}

	const auto& exp = *exp_opt;
	nlohmann::json j_exp;
	j_exp["id"] = exp.id;
	j_exp["created_at"] = exp.created_at;
	j_exp["short_name"] = exp.short_name;
	j_exp["description"] = exp.description;

	nlohmann::json j_results = nlohmann::json::array();
	const auto rows = db.GetResultsForExperiment(experiment_id);
	for (const auto& r : rows) {
		nlohmann::json j_row;
		j_row["id"] = r.id;
		j_row["input_file_id"] = r.input_file_id;
		j_row["experiment_id"] = r.experiment_id;
		j_row["filename"] = r.filename;
		j_row["vertex_count"] = r.vertex_count.has_value() ? nlohmann::json(*r.vertex_count) : nlohmann::json(nullptr);
		j_row["optimal_result"] = r.optimal_result.has_value() ? nlohmann::json(*r.optimal_result) : nlohmann::json(nullptr);
		j_row["num_colors_result"] = r.num_colors_result;
		j_row["time_taken"] = r.time_taken;
		j_row["max_iter_between"] = r.max_iter_between.has_value() ? nlohmann::json(*r.max_iter_between) : nlohmann::json(nullptr);
		j_row["max_time_between"] = r.max_time_between.has_value() ? nlohmann::json(*r.max_time_between) : nlohmann::json(nullptr);
		j_results.push_back(std::move(j_row));
	}
	j_exp["results"] = std::move(j_results);
	return j_exp;
}

void ExportComparisonAndView(ResultDatabase& db, int exp_a_id, int exp_b_id) {
	nlohmann::json payload;
	payload["experiment_a"] = ExperimentToJson(db, exp_a_id);
	payload["experiment_b"] = ExperimentToJson(db, exp_b_id);

	std::filesystem::create_directories("./export");
	const std::string out_path = "./export/exp_comparison.js";
	std::ofstream ofs(out_path);
	if (!ofs.is_open()) {
		throw std::runtime_error("Could not write comparison export file: " + out_path);
	}
	ofs << "window.EXP_COMPARISON_DATA = " << payload.dump(2) << ";\n";
	ofs.close();

	std::cout << "Wyeksportowano dane porownania do " << out_path << std::endl;
	OpenComparisonHtml();
}

void PrintAvailableExperiments(ResultDatabase& db) {
	const auto experiments = db.ListExperiments();
	if (experiments.empty()) {
		std::cout << "Brak zapisanych eksperymentow w bazie.\n";
		return;
	}
	std::cout << "\nDostepne eksperymenty:\n";
	for (const auto& exp : experiments) {
		std::cout << "  [ID: " << exp.id << "] " << exp.short_name
			<< " (" << exp.created_at << ")";
		if (!exp.description.empty()) {
			std::cout << " - " << exp.description;
		}
		std::cout << "\n";
	}
	std::cout << "\n";
}

int PromptExperimentSelection(ResultDatabase& db, const std::string& label) {
	while (true) {
		std::string input = PromptLine("Podaj ID lub nazwe dla " + label + " (lub 'list' aby wypisac eksperymenty): ");
		if (input == "list" || input == "ls") {
			PrintAvailableExperiments(db);
			continue;
		}
		auto resolved = db.ResolveExperimentId(input);
		if (resolved.has_value()) {
			return *resolved;
		}
		std::cout << "Nie znaleziono eksperymentu '" << input << "'. Sprobuj ponownie lub wpisz 'list'.\n";
	}
}

void RunAndSave(Problem& problem, ResultDatabase& db, int experiment_id) {
	std::cout << "File: " << problem.name << std::endl;
	auto& graph = *problem.graph;
	TabuSearch tabuSearch(graph);

	auto elapsed_time = MeasureTime([&] {
		tabuSearch();
		});

	int result = tabuSearch.GetUsedColors();
	int max_iter_between = tabuSearch.GetMaxIterBetween();
	double max_time_between = tabuSearch.GetMaxTimeBetween();

	std::cout << "Time around tabuSearch(): " << elapsed_time << "ms\n";
	std::cout << "Colors used: " << result
		<< " | max_iter_between: " << max_iter_between
		<< " | max_time_between: " << max_time_between << "s\n";

	db.SaveResult(problem.database_id, experiment_id, result, elapsed_time, max_iter_between, max_time_between);
}

void RunMultiple(const std::vector<std::filesystem::path>& filenames, ResultDatabase& db, int experiment_id) {
	int succeeded = 0;
	int failed = 0;
	for (const auto& filename : filenames) {
		try {
			Problem problem = Problem::Load(filename.string());
			RunAndSave(problem, db, experiment_id);
			++succeeded;
		}
		catch (const std::exception& error) {
			std::cerr << "Failed to process " << filename << ": " << error.what() << std::endl;
			++failed;
		}
	}
	std::cout << "Finished " << succeeded << " input file(s); " << failed << " failed." << std::endl;
}

void RunAllInDirectory(const std::string& directory, ResultDatabase& db, int experiment_id) {
	if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
		throw std::runtime_error("Input directory not found: " + directory);

	std::vector<std::filesystem::path> filenames;
	for (const auto& entry : std::filesystem::directory_iterator(directory)) {
		if (entry.is_regular_file()) {
			filenames.push_back(entry.path());
		}
	}
	std::sort(filenames.begin(), filenames.end());
	RunMultiple(filenames, db, experiment_id);
}

void HandleCreateNewExperiment(ResultDatabase& db) {
	std::string name = PromptLine("Podaj krotka nazwe eksperymentu (Enter = domyslna): ");
	if (name.empty()) {
		name = "tabu_exp";
	}

	std::string description = PromptLine("Podaj opis eksperymentu (Enter = domyslny): ");
	if (description.empty()) {
		description = "Seria uruchomien TabuSearch z domyslnymi parametrami";
	}

	int exp_id = db.CreateExperiment(name, description);
	std::cout << "Utworzono eksperyment ID=" << exp_id << " (" << name << ").\n";

	RunAllInDirectory("./input", db, exp_id);
}

void HandleCompareExperiments(ResultDatabase& db) {
	PrintAvailableExperiments(db);
	int exp_a = PromptExperimentSelection(db, "Eksperyment A (bazowy/starszy)");
	int exp_b = PromptExperimentSelection(db, "Eksperyment B (nowy/porownywany)");
	ExportComparisonAndView(db, exp_a, exp_b);
}

} // namespace

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	try {
		ResultDatabase db;

		while (true) {
			std::cout << "\n=== TabuSearch - Menu ===\n";
			std::cout << "1. Nowy eksperyment (uruchom na ./input i zapisz)\n";
			std::cout << "2. Porownaj dwa eksperymenty (wyeksportuj i otworz HTML)\n";
			std::cout << "0. Wyjscie\n";

			std::string choice = PromptLine("Wybierz opcje: ");
			if (choice == "1") {
				HandleCreateNewExperiment(db);
			}
			else if (choice == "2") {
				HandleCompareExperiments(db);
			}
			else if (choice == "0") {
				break;
			}
			else {
				std::cout << "Niepoprawny wybor. Wpisz 1, 2 lub 0.\n";
			}
		}
	}
	catch (const std::exception& ex) {
		std::cerr << "Blad krytyczny: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}