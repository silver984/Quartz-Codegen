#pragma once
#include <quartz/core/parsed_types.hpp>
#include <filesystem>

namespace quartz
{

parsed_class parse(const std::filesystem::path& header);

} // namespace quartz