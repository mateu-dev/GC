#include "Node.h"
#include "options.h"
#include "TabuSearch.h"
#include <chrono>
#include <climits>
#include <vector>

namespace {
struct TimerGuard {
	std::chrono::steady_clock::time_point start;
	std::chrono::steady_clock::duration& result;

	~TimerGuard() {
		result = std::chrono::steady_clock::now() - start;
	}
};
}

int GCAlgorithmInstance::CalculateCollisions() {
	int node_count = graph.GetNodeCount();
	int collisions = 0;
	for (int i = 0; i < node_count; i++) {
		Node* node = graph.GetNode(i);
		for (Node* nb : graph.GetNeighborNodes(i)) {
			if (node->color == nb->color) {
				collisions++;
			}
		}
	}
	return collisions / 2;
}

void GCAlgorithmInstance::ChangeColor(int node_index, int new_color)
{
	Node* node = graph.GetNode(node_index);
	int old_color = node->color;

	if (old_color == new_color) return;

	for (int neighbor_index : graph.GetNeighborIndexes(node_index)) {
		Node* neighbor = graph.GetNode(neighbor_index);
		if (neighbor->color == old_color) --collisions;
		if (neighbor->color == new_color) ++collisions;

		++color_lookup[neighbor_index][new_color];
		--color_lookup[neighbor_index][old_color];
	}

	node->color = new_color;
}

// Removes gaps in colors and detects if even fewer colors are actually used
void TabuSearch::CompactColors() {
	int node_count = graph.GetNodeCount();
	std::vector<bool> color_present(used_colors + 1, false);

	for (int i = 0; i < node_count; i++) {
		int c = graph.GetNode(i)->color;
		if (c >= 1 && c <= used_colors) {
			color_present[c] = true;
		}
	}

	// Remap colors to be contiguous: 1 .. new_used_colors
	std::vector<int> remap(used_colors + 1, 0);
	int new_count = 0;
	for (int c = 1; c <= used_colors; c++) {
		if (color_present[c]) {
			new_count++;
			remap[c] = new_count;
		}
	}

	for (int i = 0; i < node_count; i++) {
		int c = graph.GetNode(i)->color;
		if (c >= 1 && c <= used_colors) {
			ChangeColor(i, remap[c]);
		}
	}
	used_colors = new_count;
}

void TabuSearch::SaveColoring(std::vector<int>& dest) {
	int node_count = graph.GetNodeCount();
	if (dest.size() != (size_t)node_count) dest.resize(node_count);
	for (int i = 0; i < node_count; i++) {
		dest[i] = graph.GetNode(i)->color;
	}
}

void TabuSearch::RestoreColoring(const std::vector<int>& src) {
	int node_count = graph.GetNodeCount();
	for (int i = 0; i < node_count; i++) {
		ChangeColor(i, src[i]);
	}
}


TabuSearch::TabuSearch(Graph_& graph, int dlugosc_tabu) : GCAlgorithmInstance(graph), dlugosc_tabu(dlugosc_tabu) {}

void TabuSearch::GreedyColoring() {
	int node_count = graph.GetNodeCount();
	used_colors = 0;

	for (int i = 0; i < node_count; i++) {
		std::vector<int> neighbors = graph.GetNeighborIndexes(i);
		Node* current_node = graph.GetNode(i);
		ChangeColor(i, 0); // Reset color

		for (int color = 1; color <= used_colors; color++) {
			bool can_use_color = true;
			for (int neighbor : neighbors) {
				if (graph.GetNode(neighbor)->color == color) {
					can_use_color = false;
					break;
				}
			}
			if (can_use_color) {
				ChangeColor(i, color);
				break;
			}
		}
		if (current_node->color == 0) {
			used_colors++;
			ChangeColor(i, used_colors);
		}
	}
}

void TabuSearch::ReduceColors() {
	if (used_colors <= 1) return;

	int node_count = graph.GetNodeCount();
	std::vector<int> colors_amount_array(used_colors + 1, 0);

	for (int i = 0; i < node_count; i++) {
		int col = graph.GetNode(i)->color;
		if (col >= 1 && col <= used_colors) {
			colors_amount_array[col]++;
		}
	}

	// Choose the color used the least
	int color_to_remove = 1;
	int min_color_count = INT_MAX;
	for (int i = 1; i <= used_colors; i++) {
		if (colors_amount_array[i] < min_color_count) {
			min_color_count = colors_amount_array[i];
			color_to_remove = i;
		}
	}

	// Reassign nodes of the removed color to the best alternative color
	for (int i = 0; i < node_count; i++) {
		Node* node_ptr = graph.GetNode(i);
		if (node_ptr->color == color_to_remove) {
			int min_collisions = INT_MAX;
			int color_with_least_collisions = 1;
			std::vector<Node*> neighbours = graph.GetNeighborNodes(i);

			for (int new_color = 1; new_color <= used_colors; new_color++) {
				if (new_color == color_to_remove) continue;
				int local_collisions = 0;
				for (Node* neighbour : neighbours) {
					if (neighbour->color == new_color) local_collisions++;
				}
				if (local_collisions < min_collisions) {
					min_collisions = local_collisions;
					color_with_least_collisions = new_color;
				}
			}
			ChangeColor(i, color_with_least_collisions);
		}
	}

	// Compact immediately while used_colors still reflects the max color ID (8),
	// so CompactColors() sees all colors 1..8 and updates used_colors to 7.
	CompactColors();
}

bool TabuSearch::TabuCore() {
	int node_count = graph.GetNodeCount();
	if (collisions == 0) return true;
	if (node_count == 0) return false;

	std::vector<std::vector<int>> tabu_matrix(node_count, std::vector<int>(used_colors + 1, 0));

	int min_historical_collisions = collisions;
	int current_global_collisions = collisions;
	const auto deadline = timeStart + timeLimit - std::chrono::seconds(1);

	int iteration = 1;
	while (true) {
		if (Clock::now() >= deadline) {
			break;
		}

		int best_candidate_collisions = INT_MAX;
		int best_candidate_node = 0;
		int best_candidate_color = graph.GetNode(0)->color;

		for (int node = 0; node < node_count; node++) {
			Node* current_node = graph.GetNode(node);
			int old_color = current_node->color;
			int previous_local_collisions = color_lookup[node][old_color];

			for (int color = 1; color <= used_colors; color++) {
				if (old_color == color) continue;

				int current_after_change_local_collisions = color_lookup[node][color];

				int proposed_global_collisions =
					current_global_collisions
					- previous_local_collisions
					+ current_after_change_local_collisions;

				if (proposed_global_collisions < best_candidate_collisions) {
					bool is_tabu = (iteration <= tabu_matrix[node][color]);
					// Aspiration criterion: allow tabu move if it breaks historical best
					if (!is_tabu || proposed_global_collisions < min_historical_collisions) {
						best_candidate_collisions = proposed_global_collisions;
						best_candidate_node = node;
						best_candidate_color = color;
					}
				}
			}
		}

		if (best_candidate_collisions == INT_MAX) {
			break;
		}

		// Apply move and update Tabu matrix
		int old_color = graph.GetNode(best_candidate_node)->color;
		tabu_matrix[best_candidate_node][old_color] = iteration + dlugosc_tabu;
		ChangeColor(best_candidate_node, best_candidate_color);

		current_global_collisions = best_candidate_collisions;
		if (current_global_collisions < min_historical_collisions) {
			min_historical_collisions = current_global_collisions;
		}

		iteration++;
		iterations++;

		if (current_global_collisions == 0) {
			collisions = 0;
			return true; // Success: Found valid coloring with current colors
		}
	}

	collisions = current_global_collisions;
	return (collisions == 0);
}

void TabuSearch::run() {
	timeStart = Clock::now();
	TimerGuard guard{ timeStart, timeResult };

	iterations = 0;

	GreedyColoring();
	CompactColors();

	int best_valid_colors = used_colors;
	std::vector<int> best_coloring;
	SaveColoring(best_coloring);

	// Outer loop to continuously reduce colors and attempt Tabu optimization
	while (used_colors > 1) {
		if (Clock::now() >= timeStart + timeLimit - std::chrono::seconds(1)) {
			RestoreColoring(best_coloring);
			used_colors = best_valid_colors;
			collisions = CalculateCollisions();
			break;
		}

		ReduceColors();
		CompactColors();

		bool success = TabuCore();

		if (success) {
			// If Tabu succeeded (0 collisions), compact colors further
			// in case it naturally eliminated another color, then save state.
			CompactColors();
			best_valid_colors = used_colors;
			SaveColoring(best_coloring);
		}
		else {
			// Failed to find valid coloring at this color count (or timed out);
			// revert to the last successful coloring and stop.
			RestoreColoring(best_coloring);
			used_colors = best_valid_colors;
			collisions = CalculateCollisions();
			break;
		}
	}
}

int TabuSearch::GetUsedColors() {
	return used_colors;
}