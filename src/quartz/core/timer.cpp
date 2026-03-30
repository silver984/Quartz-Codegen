#include <quartz/core/timer.hpp>

namespace quartz
{

double end_timer(const std::chrono::steady_clock::time_point& start)
{
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	return elapsed.count();
}

} // quartz