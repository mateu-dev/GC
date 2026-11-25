#include "Measure.h"
#include "options.h"
#include "ProblemLoader.h"
#include "ResultDatabase.h"
#include "TabuSearch.h"
#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

void RunAndSave(Problem& problem) {
	std::cout << "File: " << problem.name << std::endl;
	auto& graph = *problem.graph;
	TabuSearch tabuSearch(graph);

	auto elapsed_time = MeasureTime([&] {
		tabuSearch();
		});

	std::cout << "Time around tabuSearch(): " << elapsed_time << "ms\n";

	int result = tabuSearch.GetUsedColors();
	std::cout << "Colors used: " << result << std::endl;

	ResultDatabase results;
	results.SaveResult(problem.database_id, result, elapsed_time);
}

void RunSingle(const char* filename) {
	Problem problem = Problem::Load(filename);
	RunAndSave(problem);
	auto& graph = *problem.graph;
	if (graph.GetNodeCount() < 100)
		graph.ExportAndView();
	else graph.Export("./export/graph.json");
}

void RunMultiple(const std::vector<std::filesystem::path>& filenames) {
	int succeeded = 0;
	int failed = 0;
	for (const auto& filename : filenames) {
		try {
			Problem problem = Problem::Load(filename.string());
			RunAndSave(problem);
			++succeeded;
		}
		catch (const std::exception& error) {
			std::cerr << "Failed to process " << filename << ": " << error.what() << std::endl;
			++failed;
		}
	}
	std::cout << "Finished " << succeeded << " input file(s); " << failed << " failed." << std::endl;
}

void RunAllInDirectory(const std::string& directory) {
	if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
		throw std::runtime_error("Input directory not found: " + directory);

	std::vector<std::filesystem::path> filenames;
	for (const auto& entry : std::filesystem::directory_iterator(directory)) {
		if (entry.is_regular_file()) {
			filenames.push_back(entry.path());
		}
	}
	std::sort(filenames.begin(), filenames.end());
	RunMultiple(filenames);
}

int main(int argc, char* argv[]) {
	RunAllInDirectory("./input");

	return 0;
}
