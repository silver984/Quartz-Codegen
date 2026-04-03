#pragma once
#include <filesystem>
#include <quartz/core/parsed_types.hpp>

namespace quartz::bindings {

void generate_header(const quartz::parsed_class& parsed);
void generate_impl(const quartz::parsed_class& parsed);

} // namespace quartz::bindings