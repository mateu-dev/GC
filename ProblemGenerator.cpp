#include "ProblemGenerator.h"
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <ios>
#include <iostream>

#define RANDINT(MAX) (int)(rand()%(MAX))

void GenerateProblem(const char* filename, int node_count, double conn_density, int seed) {
	srand(seed);

	std::ofstream stream(filename, std::ios_base::trunc);
	stream << node_count << "\n";

	int c = node_count * node_count * conn_density;
	int index_max = node_count - 1;
	while (c-- > 0) {
		int from = RANDINT(index_max);
		int to = RANDINT(index_max);
		if (from == to) continue;

		stream << from+1 << " " << to+1 << "\n";
	}

	stream.close();
}

void GenerateProblem(const char* filename, int node_count, double conn_density) {
	GenerateProblem(filename, node_count, conn_density, time(NULL));
}
