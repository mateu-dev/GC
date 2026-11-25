#include "Measure.h"
#include "options.h"
#include "TabuSearch.h"
#include "ProblemLoader.h"
#include <iostream>
#include "ResultDatabase.h"



int main(int argc, char* argv[]) {
	// If an argument is provided, use it; otherwise fallback to default file
	const char* filename = (argc > 1) ? argv[1] : "queen8_8.col";
	std::cout << "File: " << filename << std::endl;

	Problem problem = Problem::Load(filename);
	auto& graph = *problem.graph;

	// Instantiate TabuSearch with the graph and a tabu tenure length (e.g., 10)
	TabuSearch tabuSearch(graph);

	auto elapsed_time = MeasureTime([&] {
		tabuSearch();
		});

	std::cout << "Time around tabuSearch(): " << elapsed_time << "ms\n";

	int result = tabuSearch.GetUsedColors();
	std::cout << "Colors used: " << result << std::endl;

	ResultDatabase results;
	results.SaveResult(problem.database_id, result, elapsed_time);
	if (graph.GetNodeCount() < 100)
		graph.ExportAndView();
	else graph.Export("./export/graph.json");

	return 0;
}
