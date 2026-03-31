#pragma once
#include <vector>
#include <filesystem>

namespace quartz
{

std::vector<std::filesystem::path> headers();
bool try_create_dir(const std::filesystem::path& path);

} // quartz