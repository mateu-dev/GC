#ifndef TABUSEARCH_H
#define TABUSEARCH_H

#include <chrono>
#include <memory>
#include <vector>
#include "options.h"
#include "StopConditions.h"

class GCAlgorithmInstance {
protected:
	using Clock = std::chrono::steady_clock;

	std::vector<std::vector<int>> color_lookup;
	Graph_& graph;

	int collisions{ 0 };

	std::unique_ptr<RunCondition> run_condition = std::make_unique<IterationLimitCondition>(1000);

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
		run_condition->Reset();
		this->run();
	}
};

class TabuSearch : public GCAlgorithmInstance {
	int dlugosc_tabu{ 1 };
	int used_colors{ 0 };

	// Statystyki przerw między kolejnymi nowymi (lepszymi) rozwiązaniami w run()
	int total_iterations{ 0 };
	int max_iter_between{ 0 };
	double max_time_between{ 0.0 };

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
	int GetMaxIterBetween() const;
	double GetMaxTimeBetween() const;
};

#endif // TABUSEARCH_H