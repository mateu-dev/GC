#include "ProblemLoader.h"

#include "ResultDatabase.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
	struct ParsedInput
	{
		int vertex_count = 0;
		std::vector<std::pair<int, int>> edges;
		bool dimacs = false;
	};

	ParsedInput ParseInput(std::istream& input, const std::string& file_path)
	{
		ParsedInput parsed;
		std::string line;
		int line_number = 0;

		while (std::getline(input, line)) {
			++line_number;
			std::istringstream row(line);
			std::string first;
			if (!(row >> first))
				continue;

			if (first == "c" || first == "%" || first == "#")
				continue;

			if (first == "p") {
				std::string format;
				int edge_count = 0;
				if (!(row >> format >> parsed.vertex_count >> edge_count) || parsed.vertex_count <= 0 || edge_count < 0)
					throw std::runtime_error("Invalid DIMACS problem header at line " + std::to_string(line_number));
				parsed.dimacs = true;
				continue;
			}

			if (first == "e") {
				int from, to;
				if (!parsed.dimacs || !(row >> from >> to))
					throw std::runtime_error("Invalid DIMACS edge at line " + std::to_string(line_number));
				parsed.edges.emplace_back(from, to);
				continue;
			}

			std::istringstream first_as_number(first);
			int first_number;
			if (!(first_as_number >> first_number) || !first_as_number.eof())
				throw std::runtime_error("Unrecognized input at line " + std::to_string(line_number) + " in " + file_path);

			if (parsed.vertex_count == 0) {
				// Plain text graph files start with the number of vertices.
				parsed.vertex_count = first_number;
				if (parsed.vertex_count <= 0)
					throw std::runtime_error("Vertex count must be positive");
				continue;
			}

			int to;
			if (!(row >> to))
				throw std::runtime_error("Expected an edge endpoint at line " + std::to_string(line_number));
			std::string extra;
			if (row >> extra)
				throw std::runtime_error("Unexpected data at line " + std::to_string(line_number));
			parsed.edges.emplace_back(first_number, to);
		}

		if (parsed.vertex_count <= 0)
			throw std::runtime_error("No valid graph header found in " + file_path);

		return parsed;
	}
}

Problem Problem::Load(const std::string& file_path)
{
	std::ifstream input(file_path);
	if (!input)
		throw std::runtime_error("Could not open problem file: " + file_path);

	ParsedInput parsed = ParseInput(input, file_path);
	Problem problem;
	problem.name = std::filesystem::path(file_path).lexically_normal().string();
	problem.graph = std::make_unique<Graph_>(parsed.vertex_count);

	for (const auto& [from, to] : parsed.edges) {
		// Both supported formats use DIMACS style 1-based vertex numbers.
		if (from < 1 || from > parsed.vertex_count || to < 1 || to > parsed.vertex_count)
			throw std::runtime_error("Edge endpoint is outside the declared vertex range in " + file_path);
		if (!problem.graph->ExistsEdge(from - 1, to - 1))
			problem.graph->AddEdge(from - 1, to - 1);
	}

	ResultDatabase database;
	problem.database_id = database.RegisterOrGetInput(problem.name, problem.graph->GetNodeCount(), -1);
	return problem;
}
