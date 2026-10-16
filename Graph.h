#pragma once

#include <fstream>
#include <iostream>
#include <type_traits>
#include <vector>

#define DEBUG

#define PRINT(A)
#ifdef DEBUG
#define PRINT(A) std::cout<<A<<"\n";
#endif // DEBUG


template<typename T = void>
class Graph
{
protected:
    int node_count;

    T* nodes{ nullptr }; // Graph owns node instances
    bool* adjacency_matrix;
public:
    static Graph<T>* LoadGraph(const char* filename);

    Graph(const int& node_count);
    virtual ~Graph();

    int GetNodeCount() { return node_count; }

    void AddEdge(const int& from, const int& to);
    void RemoveEdge(const int& from, const int& to);
    bool ExistsEdge(const int& from, const int& to);

    std::vector<int> GetNeighborIndexes(const int& index);

    T* GetNode(const int& index);
    void SetNode(const int& index, const T& node);
    std::vector<T*> GetNeighborNodes(const int& index);
};

template<typename T>
Graph<T>* Graph<T>::LoadGraph(const char* filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) throw "failed to open file";

    int node_count;
    if (!(file >> node_count) || node_count <= 0)
        throw "Invalid graph";

    auto graph = new Graph<T>(node_count);

    int from, to;
    while (file >> from >> to) {
        graph->AddEdge(from, to);
    }

    return graph;
}

template<typename T>
inline Graph<T>::Graph(const int& node_count) :
    node_count(node_count),
    adjacency_matrix(new bool[node_count * node_count] {})
{
    if constexpr (!std::is_void_v<T>)
        nodes = new T[node_count] {};
}

template<typename T>
Graph<T>::~Graph()
{
    delete[] adjacency_matrix;

    if constexpr (!std::is_void_v<T>)
        delete[] nodes;
}

template<typename T>
void Graph<T>::AddEdge(const int& from, const int& to)
{
    int f = from - 1;
    int t = to - 1;
    adjacency_matrix[f * node_count + t] = 1;
    adjacency_matrix[t * node_count + f] = 1; // Undirected / nondirectional
}

template<typename T>
void Graph<T>::RemoveEdge(const int& from, const int& to)
{
    int f = from - 1;
    int t = to - 1;
    adjacency_matrix[f * node_count + t] = 0;
    adjacency_matrix[t * node_count + f] = 0;
}

template<typename T>
bool Graph<T>::ExistsEdge(const int& from, const int& to)
{
    int f = from - 1;
    int t = to - 1;
    return adjacency_matrix[f * node_count + t];
}

template<typename T>
std::vector<int> Graph<T>::GetNeighborIndexes(const int& index)
{
    std::vector<int> neighbors;
    neighbors.reserve(node_count);
    int idx = index - 1;
    for (int i = 0; i < node_count; i++) {
        if (adjacency_matrix[idx * node_count + i]) {
            neighbors.push_back(i + 1); // Return 1-based neighbor indices
        }
    }
    return neighbors;
}

template<typename T>
T* Graph<T>::GetNode(const int& index) {
    if constexpr (!std::is_void_v<T>)
        return &nodes[index - 1];
    return nullptr;
}

template<typename T>
void Graph<T>::SetNode(const int& index, const T& node) {
    if constexpr (!std::is_void_v<T>)
        nodes[index - 1] = node;
}

template<typename T>
std::vector<T*> Graph<T>::GetNeighborNodes(const int& index)
{
    std::vector<T*> neighbors;
    neighbors.reserve(node_count);
    int idx = index - 1;
    for (int i = 0; i < node_count; i++) {
        if (adjacency_matrix[idx * node_count + i]) {
            neighbors.push_back(GetNode(i + 1));
        }
    }
    return neighbors;
}