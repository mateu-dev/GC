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

	T* nodes;
	bool** adjacency_matrix;

	std::vector<std::vector<int>> index_adjacency_list;
	std::vector<std::vector<T*>> node_adjacency_list;

	bool IsInRange(const int& index) { return index >= 0 && index < node_count; }
public:
	static Graph<T>* LoadGraph(const char* filename, const bool file_indexed_from_one = false);

	Graph(const int& node_count);
	virtual ~Graph();

	int GetNodeCount() const { return node_count; }

	void AddEdge(const int& from, const int& to);
	bool ExistsEdge(const int& from, const int& to);

	const std::vector<int>& GetNeighborIndexes(const int& index);

	T* GetNode(const int& index);
	void SetNode(const int& index, const T& node);
	const std::vector<T*>& GetNeighborNodes(const int& index);

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
	adjacency_matrix(new bool* [node_count])
{

	index_adjacency_list.resize(node_count);
	for (int i = 0; i < node_count; i++) {
		adjacency_matrix[i] = new bool[node_count] {false};
		index_adjacency_list[i].reserve(node_count);
	}

	if constexpr (!std::is_void_v<T>) {
		nodes = new T[node_count]{};
		node_adjacency_list.resize(node_count);
		for (int i = 0; i < node_count; i++) {
			node_adjacency_list[i].reserve(node_count);
		}
	}

}

TEMPLATE
inline Graph<T>::~Graph()
{
	for (int i = 0; i < node_count; i++)
		delete[] adjacency_matrix[i];
	delete[] adjacency_matrix;

	if constexpr (!std::is_void_v<T>) {
		delete[] nodes;
	}
}

TEMPLATE
void Graph<T>::AddEdge(const int& from, const int& to)
{
	if (!IsInRange(from) || !IsInRange(to)) return;

	adjacency_matrix[from][to] = 1;
	adjacency_matrix[to][from] = 1;

	index_adjacency_list[from].push_back(to);
	index_adjacency_list[to].push_back(from);

	if constexpr (!std::is_void_v<T>) {
		node_adjacency_list[from].push_back(GetNode(to));
		node_adjacency_list[to].push_back(GetNode(from));
	}
}

TEMPLATE
bool Graph<T>::ExistsEdge(const int& from, const int& to)
{
	if (!IsInRange(from) || !IsInRange(to))
		return false;

	return adjacency_matrix[from][to];
}

TEMPLATE
const std::vector<int>& Graph<T>::GetNeighborIndexes(const int& index)
{
	if (!IsInRange(index)) throw "Index out of range";
	return index_adjacency_list[index];
}

TEMPLATE
T* Graph<T>::GetNode(const int& index) {
	if constexpr (!std::is_void_v<T>) {
		if (index < 0 || index >= node_count)
			return nullptr;
		return &nodes[index];
	}
	return nullptr;
}

TEMPLATE
void Graph<T>::SetNode(const int& index, const T& node) {
	if constexpr (!std::is_void_v<T>) {
		if (!IsInRange(index))
			return;
		nodes[index] = node;
	}
}

TEMPLATE
const std::vector<T*>& Graph<T>::GetNeighborNodes(const int& index)
{
	if (!IsInRange(index)) throw "Index out of range";
	return node_adjacency_list[index];
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

