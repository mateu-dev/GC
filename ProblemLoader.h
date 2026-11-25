#pragma once

#include "options.h"

#include <memory>
#include <string>

struct Problem
{
	std::string name;
	std::unique_ptr<Graph_> graph;
	int database_id = -1;

	static Problem Load(const std::string& file_path);
};
