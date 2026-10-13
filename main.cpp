#include "Graph.h"
#include "ProblemGenerator.h"

int main(int argc, char* argv[]) {
	const char* filename = "./test_input.txt"; // argv[1];

	GenerateProblem(filename, 10, 0.15);
	auto graph = Graph<void>::LoadGraph(filename);

	return 0;
}