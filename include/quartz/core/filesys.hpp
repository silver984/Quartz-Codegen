#pragma once
#include <vector>
#include <filesystem>
#include <fstream>

namespace quartz {

std::vector<std::filesystem::path> headers();
void try_create_dir(const std::filesystem::path& path);
// make sure to pass a newly initialized stream
void try_create_file(std::ofstream& stream, const std::filesystem::path& path);

} // quartz