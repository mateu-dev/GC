#ifndef TABUSEARCH_H
#define TABUSEARCH_H

#include <vector>
#include "options.h"


class GCAlgorithmInstance {
protected:
	std::vector<std::vector<int>> color_lookup;
	Graph_& graph;

	int collisions{ 0 };

	GCAlgorithmInstance(Graph_& graph) : graph(graph) {
		const int node_count = graph.GetNodeCount();
		color_lookup.resize(node_count, std::vector<int>(node_count, 0));
	}
	virtual void run() = 0;
	// Utilities
	int CalculateCollisions();
	void ChangeColor(int node_index, int new_color);
public:
	void operator ()() {
		collisions = CalculateCollisions();
		this->run();
	}
};

class TabuSearch : public GCAlgorithmInstance {
	int dlugosc_tabu{ 10 };
	int used_colors{ 0 };
	int iterations_limit{ 1000 };

	// Helper methods
	void CompactColors();
	void SaveColoring(std::vector<int>& dest);
	void RestoreColoring(const std::vector<int>& src);
	bool TabuCore();

protected:
	void run() override;
public:
	TabuSearch(Graph_& graph, int dlugosc_tabu = 10);

	void GreedyColoring();
	void ReduceColors();
	int GetUsedColors();
};

#endif // TABUSEARCH_H