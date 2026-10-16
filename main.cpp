#include "Graph.h"
#include "Measure.h"
#include "ProblemGenerator.h"
#include <iostream>
#include <filesystem>

struct Node {
    int color{ 0 };
    Node() = default;
};

// Change graph type here
#define Graph Graph<Node>

int GC(Graph* graph) {
    // Algorithm Here

    // ----------------------------------------------------------------------------
    int node_count = graph->GetNodeCount();
    int used_colors = 0;

    for (int i = 1; i <= node_count; i++) {
        std::vector<int> neighbors = graph->GetNeighborIndexes(i);
        Node* current_node = graph->GetNode(i);
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

    return used_colors;
}

int main(int argc, char* argv[]) {
    const char* filename = "graph_100.txt"; // argv[1];
    //GenerateProblem(filename, 10, 0.15, 1);

    auto graph = Graph::LoadGraph(filename);
    int result{};
    std::cout << "Time: " << MeasureTime([&] {
        // Run algorithm here
        result = GC(graph);
        }) << "ms\n";
    std::cout << "Result: " << result << std::endl;
    //for (int i = 1; i <= graph->GetNodeCount(); i++) {
    //    std::cout << "Node " << i << ": Color " << graph->GetNode(i)->color << "\n";
    //    for (int neighbor : graph->GetNeighborIndexes(i)) {
    //        std::cout << "  Neighbor " << neighbor << ": Color " << graph->GetNode(neighbor)->color << "\n";
    //    }
    //}

    delete graph;

    return 0;
}