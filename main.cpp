#include "Graph.h"
#include "Measure.h"
#include "json.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>

struct Node :public _Base {
	int color{ 0 };
	Node() = default;

	nlohmann::json Serialize() override {
		nlohmann::json json;
		json["color"] = color;
		return json;
	}
};

// Change graph type here
#define Graph Graph<Node>

int GC(Graph* graph) {
	const int node_count = graph->GetNodeCount();

	// We know the answers right away for size 1 and 2
	if (node_count <= 2) {
		if (node_count == 1) return 1;
		else if (graph->ExistsEdge(0, 1)) return 2;
	}

	graph->GetNode(0)->color = 0; // We dont have to check the neighbors of the first node.

	int color_count = 1;
	for (int i = 1;i < node_count;i++) {
		std::vector<bool> used_colors{ false };
		used_colors.resize(node_count);

		// Find used colors
		auto neighbors = graph->GetNeighborNodes(i);
		for (Node* neighbor : neighbors) {
			int neighbor_color = neighbor->color;
			if (neighbor_color >= 0)
				used_colors[neighbor_color] = true;
		}

		for (int j = 0;j < node_count;j++) {
			if (!used_colors[j]) {
				graph->GetNode(i)->color = j;

				// color count is 1 more than index
				int estimated_color_count = j + 1;
				// max(color_count,estimated_color_count)
				color_count = color_count < estimated_color_count ? estimated_color_count : color_count;
				break;
			}
		}
	}

	return color_count;
}

int main(int argc, char* argv[]) {
	const char* filename = "./problems/graph_20.txt"; // argv[1];

	auto graph = Graph::LoadGraph(filename, true);
	int result{};
	std::cout << "Time: " << MeasureTime([&] {
		// Run algorithm here
		result = GC(graph);
		}) << "ms\n";
	std::cout << "Result: " << result << std::endl;

	// Show graph in browser
	graph->ExportAndView();

	return 0;
}