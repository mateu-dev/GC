#include "Graph.h"
#include "Measure.h"
#include "ProblemGenerator.h"
#include <iostream>

int main(int argc, char* argv[]) {
	const char* filename = "./test_input.txt"; // argv[1];

	std::cout << "time: " << MeasureTime([&] { GenerateProblem(filename, 23, 0.15);}) << "\n";
	auto graph = Graph<void>::LoadGraph(filename);

	return 0;
}