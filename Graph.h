#pragma once

#include <fstream>
#include <iostream>
#include <type_traits>

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

	T** nodes;
	bool* adjacency_matrix;
public:
	static Graph<T>* LoadGraph(const char* filename);

	Graph(const int& node_count);
	virtual ~Graph();

	void AddEdge(const int& from, const int& to);
	void RemoveEdge(const int& from, const int& to);
	bool ExistsEdge(const int& from, const int& to);

	T* GetNode(const int& index) { return nodes[index]; }
	void SetNode(const int& index, T* node) { nodes[index] = node; }
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
		PRINT(from << " -> " << to);
	}

	return graph;
}

template<typename T>
inline Graph<T>::Graph(const int& node_count) :
	node_count(node_count),
	adjacency_matrix(new bool[node_count * node_count] {})
{
	if constexpr (!std::is_void_v<T>)
		nodes = new T * [node_count];
}

template<typename T>
Graph<T>::~Graph()
{
	delete[] nodes;
	delete[] adjacency_matrix;
}

template<typename T>
void Graph<T>::AddEdge(const int& from, const int& to)
{
	adjacency_matrix[from * node_count + to] = 1;
	adjacency_matrix[to * node_count + from] = 1;
}

template<typename T>
void Graph<T>::RemoveEdge(const int& from, const int& to)
{
	adjacency_matrix[from * node_count + to] = 0;
	adjacency_matrix[to * node_count + from] = 0;
}

template<typename T>
bool Graph<T>::ExistsEdge(const int& from, const int& to)
{
	return adjacency_matrix[from * node_count + to];
}

