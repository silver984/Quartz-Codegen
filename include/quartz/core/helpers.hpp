#pragma once
#include <string>
#include <filesystem>
#include <chrono>
#include <cstdint>

namespace quartz
{

inline std::chrono::steady_clock::time_point start_timer()
{
	return std::chrono::high_resolution_clock::now();
}

std::string pascal_to_camel(const std::string& str);
std::string camel_to_snake(const std::string& str);
std::string remove_prefix(const std::string& str, const std::string& prefix);
std::string get_namespace(const std::filesystem::path& header);
std::string indent_lines(const std::string& str, size_t spaces);
double end_timer(const std::chrono::steady_clock::time_point& start); // returns in seconds
void remove_trailing_end(std::string& str, size_t count);

} // namespace quartz