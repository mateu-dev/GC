#ifndef TABUSEARCH_H
#define TABUSEARCH_H

#include "Graph.h"
#include "Node.h"
#include <vector>

using GraphType = Graph<Node>;

class TabuSearch {
	int dlugosc_tabu;
	GraphType* graph;
	int used_colors{ 0 };
	int global_collisions{ 0 };
	int iterations_limit{ 1000 };

	// Helper methods
	int CalculateCollisions();
	void CompactColors();
	void SaveColoring(std::vector<int>& dest);
	void RestoreColoring(const std::vector<int>& src);
	bool TabuCore();

public:
	TabuSearch(GraphType* graph);
	TabuSearch(GraphType* graph, int dlugosc_tabu);

	void GreedyColoring();
	void ReduceColors();
	GraphType* run();
	int GetUsedColors();
};

#endif // TABUSEARCH_H