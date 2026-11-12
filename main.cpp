#include "Graph.h"
#include "Measure.h"
#include "Node.h"
#include "TabuSearch.h"
#include <iostream>

// Change graph type here
#define Graph Graph<Node>

int main(int argc, char* argv[]) {
	// If an argument is provided, use it; otherwise fallback to default file
	const char* filename = (argc > 1) ? argv[1] : "input/graph_20.txt";
	std::cout << "File: " << filename << std::endl;

	auto graph = Graph::LoadGraph(filename, true);
	if (!graph) {
		std::cerr << "Error: Could not load graph from " << filename << std::endl;
		return 1;
	}

	// Instantiate TabuSearch with the graph and a tabu tenure length (e.g., 10)
	TabuSearch tabuSearch(graph, 10);

	auto elapsed_time = MeasureTime([&] {
		tabuSearch.run();
		});

	std::cout << "Time: " << elapsed_time << "ms\n";

	int result = tabuSearch.GetUsedColors();
	std::cout << "Colors used: " << result << std::endl;

	graph->ExportAndView();

	delete graph;

	return 0;
}