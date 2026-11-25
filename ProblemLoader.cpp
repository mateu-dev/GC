#include "ProblemLoader.h"

#include "ResultDatabase.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
	struct ParsedInput
	{
		int vertex_count = 0;
		int declared_edge_count = -1;
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
				parsed.declared_edge_count = edge_count;
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

	ParsedInput ParseBinaryInput(const std::string& file_path)
	{
		std::ifstream input(file_path, std::ios::binary);
		if (!input)
			throw std::runtime_error("Could not open problem file: " + file_path);

		std::string length_line;
		if (!std::getline(input, length_line))
			throw std::runtime_error("Missing binary DIMACS preamble length in " + file_path);
		std::size_t parsed_characters = 0;
		const unsigned long long preamble_length = std::stoull(length_line, &parsed_characters);
		if (parsed_characters != length_line.find_last_not_of(" \t\r") + 1 || preamble_length > 16 * 1024 * 1024)
			throw std::runtime_error("Invalid binary DIMACS preamble length in " + file_path);

		std::string preamble(static_cast<std::size_t>(preamble_length), '\0');
		input.read(preamble.data(), static_cast<std::streamsize>(preamble.size()));
		if (input.gcount() != static_cast<std::streamsize>(preamble.size()))
			throw std::runtime_error("Truncated binary DIMACS preamble in " + file_path);

		std::istringstream preamble_stream(preamble);
		ParsedInput parsed = ParseInput(preamble_stream, file_path);
		if (!parsed.dimacs)
			throw std::runtime_error("Missing DIMACS problem header in " + file_path);

		int decoded_edge_count = 0;
		for (int i = 0; i < parsed.vertex_count; ++i) {
			const std::size_t row_size = static_cast<std::size_t>((i + 8) / 8);
			std::vector<unsigned char> row(row_size);
			input.read(reinterpret_cast<char*>(row.data()), static_cast<std::streamsize>(row.size()));
			if (input.gcount() != static_cast<std::streamsize>(row.size()))
				throw std::runtime_error("Truncated binary DIMACS adjacency block in " + file_path);

			for (int j = 0; j <= i; ++j) {
				const unsigned char mask = static_cast<unsigned char>(0x80u >> (j & 7));
				if ((row[static_cast<std::size_t>(j / 8)] & mask) == 0 || j == i)
					continue;
				parsed.edges.emplace_back(i + 1, j + 1);
				++decoded_edge_count;
			}
		}

		if (parsed.declared_edge_count >= 0 && decoded_edge_count != parsed.declared_edge_count)
			throw std::runtime_error("Binary DIMACS edge count does not match its preamble in " + file_path);
		return parsed;
	}

	int OptimalColorCount(const std::string& file_path)
	{
		static const std::unordered_map<std::string, int> known_values = {
			{"flat1000_50_0.col.b", 50}, {"flat1000_60_0.col.b", 60}, {"flat1000_76_0.col.b", 76},
			{"flat300_20_0.col.b", 20}, {"flat300_26_0.col.b", 26}, {"flat300_28_0.col.b", 28},
			{"fpsol2.i.1.col", 65}, {"fpsol2.i.2.col", 30}, {"fpsol2.i.3.col", 30},
			{"inithx.i.1.col", 54}, {"inithx.i.2.col", 31}, {"inithx.i.3.col", 31},
			{"le450_15a.col", 15}, {"le450_15b.col", 15}, {"le450_15c.col", 15}, {"le450_15d.col", 15},
			{"le450_25a.col", 25}, {"le450_25b.col", 25}, {"le450_25c.col", 25}, {"le450_25d.col", 25},
			{"le450_5a.col", 5}, {"le450_5a.txt", 5}, {"le450_5b.col", 5}, {"le450_5c.col", 5}, {"le450_5d.col", 5},
			{"mulsol.i.1.col", 49}, {"mulsol.i.2.col", 31}, {"mulsol.i.3.col", 31}, {"mulsol.i.4.col", 31}, {"mulsol.i.5.col", 31},
			{"zeroin.i.1.col", 49}, {"zeroin.i.2.col", 30}, {"zeroin.i.3.col", 30},
			{"anna.col", 11}, {"david.col", 11}, {"homer.col", 13}, {"huck.col", 11}, {"jean.col", 10}, {"games120.col", 9},
			{"miles1000.col", 42}, {"miles1500.col", 73}, {"miles250.col", 8}, {"miles250.txt", 8}, {"miles500.col", 20}, {"miles750.col", 31},
			{"queen11_11.col", 11}, {"queen13_13.col", 13}, {"queen5_5.col", 5}, {"queen6_6.col", 7}, {"queen6.txt", 7},
			{"queen7_7.col", 7}, {"queen8_12.col", 12}, {"queen8_8.col", 9}, {"queen9_9.col", 10},
			{"myciel3.col", 4}, {"myciel4.col", 5}, {"myciel5.col", 6}, {"myciel6.col", 7}, {"myciel7.col", 8}
		};

	const std::string basename = std::filesystem::path(file_path).filename().string();
	const auto found = known_values.find(basename);
	return found == known_values.end() ? -1 : found->second;
	}
}

Problem Problem::Load(const std::string& file_path)
{
	ParsedInput parsed;
	if (std::filesystem::path(file_path).extension() == ".b") {
		parsed = ParseBinaryInput(file_path);
	}
	else {
		std::ifstream input(file_path);
		if (!input)
			throw std::runtime_error("Could not open problem file: " + file_path);
		parsed = ParseInput(input, file_path);
	}
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
	problem.database_id = database.RegisterOrGetInput(problem.name, problem.graph->GetNodeCount(), OptimalColorCount(file_path));
	return problem;
}
