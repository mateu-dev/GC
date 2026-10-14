#pragma once

#include <functional>
#include <chrono>

int MeasureTime(std::function<void()> function) {
	auto clock = std::chrono::high_resolution_clock();
	auto start = clock.now();

	function();

	std::chrono::duration time = clock.now() - start;
	return time.count();
}