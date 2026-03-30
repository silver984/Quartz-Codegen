#pragma once
#include <chrono>

namespace quartz
{

inline std::chrono::steady_clock::time_point start_timer()
{
	return std::chrono::high_resolution_clock::now();
}

// returns in seconds
double end_timer(const std::chrono::steady_clock::time_point& start);

} // quartz