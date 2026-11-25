#ifndef TABUSEARCH_H
#define TABUSEARCH_H

#include <chrono>
#include <vector>
#include "options.h"

class GCAlgorithmInstance {
protected:
	using Clock = std::chrono::steady_clock;

	std::vector<std::vector<int>> color_lookup;
	Graph_& graph;

	int collisions{ 0 };

	Clock::time_point timeStart{};
	Clock::duration timeLastImprovement{ Clock::duration::zero() };
	Clock::duration timeResult{ Clock::duration::zero() };
	Clock::duration timeLimit{ std::chrono::minutes(3) };

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
	int dlugosc_tabu{ 1 };
	int used_colors{ 0 };
	int iterations{ 0 };

	// Helper methods
	void CompactColors();
	void SaveColoring(std::vector<int>& dest);
	void RestoreColoring(const std::vector<int>& src);
	bool TabuCore();

protected:
	void run() override;

public:
	TabuSearch(Graph_& graph);

	void GreedyColoring();
	void ReduceColors();
	int GetUsedColors();
	int GetIterations() const { return iterations; }
	Clock::duration GetTimeLastImprovement() const { return timeLastImprovement; }
	Clock::duration GetTimeResult() const { return timeResult; }
};

#endif // TABUSEARCH_H