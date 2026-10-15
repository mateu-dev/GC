#include "Graph.h"
#include "Measure.h"
#include <iostream>

// Change graph type here
#define Graph Graph<void>

int GC(const Graph* graph) {
	// Algorithm Here

	return 0;
}

int main(int argc, char* argv[]) {
	const char* filename = "./test_input.txt"; // argv[1];

	auto graph = Graph::LoadGraph(filename);
	int result{};
	std::cout << "Time: " << MeasureTime([&] {
		// Run algorithm here
		result = GC(graph);
		}) << "ms\n";
	std::cout << "Result: " << result << std::endl;

	return 0;
}