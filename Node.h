#pragma once
#include "Graph.h"
#include "json.hpp"

struct Node :public _Base {
	int color{ 0 };
	Node() = default;

	nlohmann::json Serialize() override {
		nlohmann::json json;
		json["color"] = color;
		return json;
	}
};