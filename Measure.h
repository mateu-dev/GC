#pragma once

#include <functional>
#include <chrono>

template<typename Unit = std::chrono::milliseconds>
double MeasureTime(const std::function<void()>& function) {
    using clock = std::chrono::high_resolution_clock;

    auto start = clock::now();
    function();
    auto end = clock::now();

    auto time = std::chrono::duration<double, typename Unit::period>(end - start);
    return time.count();
}
