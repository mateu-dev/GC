#include "graph.h"

int main(int argc, char* argv[]) {
	const char* filename = "./test_input.txt"; // argv[1];

	auto graph = Graph<void>::LoadGraph(filename);

	return 0;
}