#include "ProblemGenerator.h"
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <ios>
#include <iostream>
#include <filesystem>

#define RANDINT(MAX) (int)(rand()%(MAX))

void GenerateProblem(const char* filename, int node_count, double conn_density, int seed) {
    srand(seed);

    std::ofstream stream(filename, std::ios_base::trunc);
    stream << node_count << "\n";

		
    // Generates 1-based undirected edges
    for (int from = 1; from <= node_count; from++) {
        for (int to = from + 1; to <= node_count; to++) {
            if ((double)rand() / RAND_MAX < conn_density) {
                stream << from << " " << to << "\n";
            }
        }
    }

    stream.close();
}

void GenerateProblem(const char* filename, int node_count, double conn_density) {
    GenerateProblem(filename, node_count, conn_density, time(NULL));
}