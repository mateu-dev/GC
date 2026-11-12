#include "TabuSearch.h"

// Helper to calculate exact edge collisions in the graph
int TabuSearch::CalculateCollisions() {
    int node_count = graph->GetNodeCount();
    int collisions = 0;
    for (int i = 1; i <= node_count; i++) {
        Node* node = graph->GetNode(i);
        std::vector<Node*> neighbours = graph->GetNeighborNodes(i);
        for (Node* nb : neighbours) {
            if (node->color == nb->color) {
                collisions++;
            }
        }
    }
    return collisions / 2; // Undirected graph edges counted twice
}

// Removes gaps in colors and detects if even fewer colors are actually used
void TabuSearch::CompactColors() {
    int node_count = graph->GetNodeCount();
    std::vector<bool> color_present(used_colors + 1, false);
    
    for (int i = 1; i <= node_count; i++) {
        int c = graph->GetNode(i)->color;
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

    for (int i = 1; i <= node_count; i++) {
        int c = graph->GetNode(i)->color;
        if (c >= 1 && c <= used_colors) {
            graph->GetNode(i)->color = remap[c];
        }
    }
    used_colors = new_count;
    global_collisions = CalculateCollisions();
}

void TabuSearch::SaveColoring(std::vector<int>& dest) {
    int node_count = graph->GetNodeCount();
    if (dest.size() <= (size_t)node_count) dest.resize(node_count + 1);
    for (int i = 1; i <= node_count; i++) {
        dest[i] = graph->GetNode(i)->color;
    }
}

void TabuSearch::RestoreColoring(const std::vector<int>& src) {
    int node_count = graph->GetNodeCount();
    for (int i = 1; i <= node_count; i++) {
        graph->GetNode(i)->color = src[i];
    }
}

TabuSearch::TabuSearch(GraphType* graph) : graph(graph), dlugosc_tabu(10) {}

TabuSearch::TabuSearch(GraphType* graph, int dlugosc_tabu) : graph(graph), dlugosc_tabu(dlugosc_tabu) {}

void TabuSearch::GreedyColoring() {
    int node_count = graph->GetNodeCount();
    used_colors = 0;

    for (int i = 1; i <= node_count; i++) {
        std::vector<int> neighbors = graph->GetNeighborIndexes(i);
        Node* current_node = graph->GetNode(i);
        current_node->color = 0; // Reset color

        for (int color = 1; color <= used_colors; color++) {
            bool can_use_color = true;
            for (int neighbor : neighbors) {
                if (graph->GetNode(neighbor)->color == color) {
                    can_use_color = false;
                    break;
                }
            }
            if (can_use_color) {
                current_node->color = color;
                break;
            }
        }
        if (current_node->color == 0) {
            used_colors++;
            current_node->color = used_colors;
        }
    }
    global_collisions = CalculateCollisions();
}

void TabuSearch::ReduceColors() {
    if (used_colors <= 1) return;

    int node_count = graph->GetNodeCount();
    std::vector<int> colors_amount_array(used_colors + 1, 0);
    
    for (int i = 1; i <= node_count; i++) {
        int col = graph->GetNode(i)->color;
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
    for (int i = 1; i <= node_count; i++) {
        Node* node_ptr = graph->GetNode(i);
        if (node_ptr->color == color_to_remove) {
            int min_collisions = INT_MAX;
            int color_with_least_collisions = 1;
            std::vector<Node*> neighbours = graph->GetNeighborNodes(i);

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
            node_ptr->color = color_with_least_collisions;
        }
    }

    used_colors--;
    global_collisions = CalculateCollisions();
}

bool TabuSearch::TabuCore() {
    int node_count = graph->GetNodeCount();
    global_collisions = CalculateCollisions();
    if (global_collisions == 0) return true;

    std::vector<std::vector<int>> tabu_matrix(node_count + 1, std::vector<int>(used_colors + 1, 0));

    int min_historical_collisions = global_collisions;
    int current_global_collisions = global_collisions;

    for (int iteration = 1; iteration <= iterations_limit; iteration++) {
        int min_global_collisions = INT_MAX;
        int min_global_collisions_node = 1;
        int min_global_collisions_color = graph->GetNode(1)->color;

        for (int node = 1; node <= node_count; node++) {
            Node* current_node = graph->GetNode(node);
            int old_color = current_node->color;
            std::vector<Node*> neighbours = graph->GetNeighborNodes(node);

            int previous_local_collisions = 0;
            for (Node* neighbour : neighbours) {
                if (old_color == neighbour->color) previous_local_collisions++;
            }

            for (int color = 1; color <= used_colors; color++) {
                if (old_color == color) continue;

                int current_after_change_local_collisions = 0;
                for (Node* neighbour : neighbours) {
                    if (color == neighbour->color) current_after_change_local_collisions++;
                }

                int proposed_global_collisions = current_global_collisions - 
                    (previous_local_collisions - current_after_change_local_collisions);

                if (proposed_global_collisions < min_global_collisions) {
                    bool is_tabu = (iteration <= tabu_matrix[node][color]);
                    // Aspiration criterion: allow tabu move if it breaks historical best
                    if (!is_tabu || proposed_global_collisions < min_historical_collisions) {
                        min_global_collisions = proposed_global_collisions;
                        min_global_collisions_node = node;
                        min_global_collisions_color = color;
                    }
                }
            }
        }

        if (min_global_collisions == INT_MAX) break;

        // Apply move and update Tabu matrix
        int old_color = graph->GetNode(min_global_collisions_node)->color;
        tabu_matrix[min_global_collisions_node][old_color] = iteration + dlugosc_tabu;
        graph->GetNode(min_global_collisions_node)->color = min_global_collisions_color;

        current_global_collisions = min_global_collisions;
        if (current_global_collisions < min_historical_collisions) {
            min_historical_collisions = current_global_collisions;
        }

        if (current_global_collisions == 0) {
            global_collisions = 0;
            return true; // Success: Found valid coloring with current colors
        }
    }

    global_collisions = current_global_collisions;
    return (global_collisions == 0);
}

GraphType* TabuSearch::run() {
    GreedyColoring();
    CompactColors();

    int best_valid_colors = used_colors;
    std::vector<int> best_coloring;
    SaveColoring(best_coloring);

    // Outer loop to continuously reduce colors and attempt Tabu optimization
    while (used_colors > 1) {
        ReduceColors();
        CompactColors();

        bool success = TabuCore();

        if (success) {
            // If Tabu succeeded (0 collisions), compact colors further 
            // in case it naturally eliminated another color, then save state.
            CompactColors();
            best_valid_colors = used_colors;
            SaveColoring(best_coloring);
        } else {
            // Failed to find valid coloring at this color count; 
            // revert to the last successful coloring and stop.
            RestoreColoring(best_coloring);
            used_colors = best_valid_colors;
            break;
        }
    }

    return graph;
}

int TabuSearch::GetUsedColors() {
    return used_colors;
}