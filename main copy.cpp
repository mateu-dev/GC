#include "Graph.h"
#include "Measure.h"
#include <iostream>

struct Node {
	int color{ 0 };
	Node() = default;
};

// Change graph type here
#define Graph Graph<Node>

int GC(const Graph* graph) {
	// Algorithm Here

	// Example: Set Node's value
	// graph->GetNode(4)->color = 2;

	// Example: Print Node Color
	// std::cout << graph->GetNode(4)->color << "\n";
	
	return -1;
}

int main(int argc, char* argv[]) {
	const char* filename = "./the_input.txt"; // argv[1];

	auto graph = Graph::LoadGraph(filename, true);
	int result{};
	std::cout << "Time: " << MeasureTime([&] {
		// Run algorithm here
		result = GC(graph);
		}) << "ms\n";
	std::cout << "Result: " << result << std::endl;


	return 0;
}