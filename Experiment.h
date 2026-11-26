#pragma once

#include <string>
#include <optional>

struct Experiment {
	int id = 0;
	std::string created_at;
	std::string short_name;
	std::string description;
};

struct ExperimentResultRow {
	int id = 0;
	int input_file_id = 0;
	int experiment_id = 0;
	std::string filename;
	std::optional<int> vertex_count;
	std::optional<int> optimal_result;
	int num_colors_result = 0;
	double time_taken = 0.0;
	std::optional<int> max_iter_between;
	std::optional<double> max_time_between;
};