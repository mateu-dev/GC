#ifndef TABUSEARCH_H
#define TABUSEARCH_H

#include <chrono>
#include <memory>
#include <vector>
#include "options.h"
#include "StopConditions.h"
#include <unordered_set>

using Coloring = std::vector<int>;

struct RunResult {
	struct ColoringSnapshot {
		int initial_color_count;
		Coloring initial_coloring;
	};

	ColoringSnapshot initial_coloring;
	ColoringSnapshot final_coloring;

	std::vector<ColoringSnapshot> saved_colorings;
};

class GCAlgorithmInstance {
	bool run_finished{ false };
	RunResult result;
protected:
	using Clock = std::chrono::steady_clock;

	std::vector<std::vector<int>> color_lookup;
	Graph_& graph;

	int collisions{ 0 };

	std::unique_ptr<RunCondition> run_condition = std::make_unique<IterationLimitCondition>(1000);
	std::vector<Coloring> saved_colorings;


	GCAlgorithmInstance(Graph_& graph) : graph(graph) {
		const int node_count = graph.GetNodeCount();
		color_lookup.resize(node_count, std::vector<int>(node_count + 1, 0));
	}
	virtual void run() = 0;
	// Utilities
	int CalculateCollisions();
	void ChangeColor(int node_index, int new_color);

	int SaveColoring();
	void RestoreLastColoring();
	void RestoreColoring(const int& cid);

	int CountColors(const Coloring& coloring) {
		std::unordered_set<int> used;
		used.reserve(coloring.size());
		for (int c : coloring)
			used.emplace(c);

		return used.size();
	}
public:
	void operator ()() {
		if (run_finished) return;

		collisions = CalculateCollisions();
		run_condition->Reset();

		this->run();

		run_finished = true;
	}

	const RunResult GetRunData() {
		if (!run_finished) throw "No run completed";

		auto& snapshots = result.saved_colorings;
		snapshots.reserve(saved_colorings.size());
		for (int i = 0;i < saved_colorings.size();i++) {
			Coloring snapshot = saved_colorings[i];
			snapshots[i] = {
				CountColors(snapshot),
				snapshot
			};
		}

		result.initial_coloring = snapshots[0];
		result.final_coloring = snapshots[snapshots.size() - 1];

		return result;
	}
};

class TabuSearch : public GCAlgorithmInstance {
	int dlugosc_tabu{ 10 };
	int used_colors{ 0 };

	// Statystyki przerw między kolejnymi nowymi (lepszymi) rozwiązaniami w run()
	int total_iterations{ 0 };
	int max_iter_between{ 0 };
	double max_time_between{ 0.0 };

	// Helper methods
	void RemapColors();
	bool TabuCore();

protected:
	void run() override;

public:
	TabuSearch(Graph_& graph);

	void GreedyColoring();
	void RandomColoring();
	void ReduceColors();
	int GetUsedColors();
	int GetMaxIterBetween() const;
	double GetMaxTimeBetween() const;
};

#endif // TABUSEARCH_H