#include "Measure.h"
#include "options.h"
#include "TabuSearch.h"
#include <iostream>
#include "ResultDatabase.h"

int main(int argc, char* argv[]) {
	// If an argument is provided, use it; otherwise fallback to default file
	const char* filename = (argc > 1) ? argv[1] : "input/queen6.txt";
	std::cout << "File: " << filename << std::endl;

	auto graph = Graph_::LoadGraph(filename, true);
	if (!graph) {
		std::cerr << "Error: Could not load graph from " << filename << std::endl;
		return 1;
	}

	// Instantiate TabuSearch with the graph and a tabu tenure length (e.g., 10)
	TabuSearch tabuSearch(*graph);

	auto elapsed_time = MeasureTime([&] {
		tabuSearch();
		});

	std::cout << "Time around tabuSearch(): " << elapsed_time << "ms\n";
	std::cout << "Time returned by GetTimeResult(): " << tabuSearch.GetTimeResult() << "\n";
	std::cout << "Time returned by GetTimeLastImprovement(): " << tabuSearch.GetTimeLastImprovement() << "\n";

	int result = tabuSearch.GetUsedColors();
	std::cout << "Colors used: " << result << std::endl;

	ResultDatabase results;
	int input_id = results.RegisterInput(filename, graph->GetNodeCount(), -1);
	results.SaveResult(input_id, result, elapsed_time);
	if (graph->GetNodeCount() < 100)
		graph->ExportAndView();
	else graph->Export("./export/graph.json");

	delete graph;

	return 0;
}