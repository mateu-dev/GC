#pragma once

#include "options.h"

#include <fstream>
#include <string>
#include <type_traits>
#include <vector>

#ifdef DEBUG
#include "json.hpp"
#endif // DEBUG

#define TEMPLATE template<typename T> requires (std::is_void_v<T> || std::is_base_of_v<_Base, T>)

struct _Base {
	virtual nlohmann::json Serialize() = 0;
};

TEMPLATE
class Graph
{
protected:
	int node_count;

	T* nodes{ nullptr }; // Graph owns node instances
	bool* adjacency_matrix;
public:
	static Graph<T>* LoadGraph(const char* filename, const bool indexed_from_one = false);

	Graph(const int& node_count);
	virtual ~Graph();

	int GetNodeCount() const { return node_count; }

	void AddEdge(const int& from, const int& to);
	void RemoveEdge(const int& from, const int& to);
	bool ExistsEdge(const int& from, const int& to);

	std::vector<int> GetNeighborIndexes(const int& index);

	T* GetNode(const int& index);
	void SetNode(const int& index, const T& node);
	std::vector<T*> GetNeighborNodes(const int& index);

	void Export(const char* filename, const bool export_as_js = false);
	void ExportAndView();
};

TEMPLATE
Graph<T>* Graph<T>::LoadGraph(const char* filename, const bool indexed_from_one)
{
	std::ifstream file(filename);
	if (!file.is_open()) throw "failed to open file";

	int node_count;
	if (!(file >> node_count) || node_count <= 0)
		throw "Invalid graph";

	auto graph = new Graph<T>(node_count);

	int from, to;
	while (file >> from >> to) {
		if (indexed_from_one)
			graph->AddEdge(from - 1, to - 1);
		else graph->AddEdge(from, to);
	}

	return graph;
}

TEMPLATE
inline Graph<T>::Graph(const int& node_count) :
	node_count(node_count),
	adjacency_matrix(new bool[node_count * node_count] {false})
{
	if constexpr (!std::is_void_v<T>)
		nodes = new T[node_count]{};
}

TEMPLATE
Graph<T>::~Graph()
{
	delete[] adjacency_matrix;

	if constexpr (!std::is_void_v<T>)
		delete[] nodes;
}

TEMPLATE
void Graph<T>::AddEdge(const int& from, const int& to)
{
	adjacency_matrix[from * node_count + to] = 1;
	adjacency_matrix[to * node_count + from] = 1;
}

TEMPLATE
void Graph<T>::RemoveEdge(const int& from, const int& to)
{
	adjacency_matrix[from * node_count + to] = 0;
	adjacency_matrix[to * node_count + from] = 0;
}

TEMPLATE
bool Graph<T>::ExistsEdge(const int& from, const int& to)
{
	return adjacency_matrix[from * node_count + to];
}

TEMPLATE
std::vector<int> Graph<T>::GetNeighborIndexes(const int& index)
{
	std::vector<int> neighbors;
	neighbors.reserve(node_count);
	for (int i = 0;i < node_count;i++)
		if (ExistsEdge(index, i))
			neighbors.push_back(i);
	return neighbors;
}

TEMPLATE
T* Graph<T>::GetNode(const int& index) {
	if constexpr (!std::is_void_v<T>)
		return &nodes[index];
}

TEMPLATE
void Graph<T>::SetNode(const int& index, const T& node) {
	if constexpr (!std::is_void_v<T>)
		nodes[index] = node;
}

TEMPLATE
std::vector<T*> Graph<T>::GetNeighborNodes(const int& index)
{
	std::vector<T*> neighbors;
	neighbors.reserve(node_count);
	for (int i = 0;i < node_count;i++)
		if (ExistsEdge(index, i))
			neighbors.push_back(GetNode(i));
	return neighbors;

}

TEMPLATE
void Graph<T>::Export(const char* filename, const bool export_as_js)
{
	nlohmann::json data;

	data["node_count"] = node_count;

	if constexpr (!std::is_void_v<T>) {
		data["nodes"] = nlohmann::json::object();
		for (int i = 0; i < node_count; ++i) {
			T* node = GetNode(i);
			if (node)
				data["nodes"][std::to_string(i)] = node->Serialize();
			else
				data["nodes"][std::to_string(i)] = nullptr;
		}
	}

	data["edges"] = nlohmann::json::object();
	for (int i = 0; i < node_count; ++i) {
		auto neighbors = GetNeighborIndexes(i);
		nlohmann::json jneighbors = nlohmann::json::array();
		for (int n : neighbors)
			jneighbors.push_back(n);

		data["edges"][std::to_string(i)] = jneighbors;
	}


	std::ofstream ofs(filename);
	if (ofs.is_open()) {
		if (export_as_js)
			ofs << "window.GRAPH_DATA = ";
		ofs << data.dump();
		if (export_as_js)
			ofs << ";";
		ofs.close();
	}
}

TEMPLATE
inline void Graph<T>::ExportAndView()
{
	const char* filename = "./export/export.js";
	Export(filename, true);
	system("start ./export/view_graph.html");
}

