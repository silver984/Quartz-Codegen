#include <quartz/codegen/bindings.hpp>
#include <fmt/format.h>
#include <filesystem>
#include <string>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <cstdint>

namespace quartz
{
namespace bindings
{

std::string make_hpp(const std::string& class_name, const std::string& ns)
{
    if (ns.empty())
    {
        const std::string tmpl = R"(#pragma once

namespace quartz
{{

class {class_name}Bindings
{{
public:
    {class_name}Bindings();
    ~{class_name}Bindings() = default;
}};

// queued bindings at static initialization time
static const {class_name}Bindings s_{class_name};

}} // quartz)";

        return fmt::format(fmt::runtime(tmpl),
                           fmt::arg("class_name", class_name));
    }

    const std::string namespaced_tmpl = R"(#pragma once

namespace quartz
{{
namespace {ns}
{{

class {class_name}Bindings
{{
public:
    {class_name}Bindings();
    ~{class_name}Bindings() = default;
}};

// queued bindings at static initialization time
static const {class_name}Bindings s_{class_name};

}} // {ns}
}} // quartz)";

    return fmt::format(fmt::runtime(namespaced_tmpl),
                       fmt::arg("ns", ns),
                       fmt::arg("class_name", class_name));
}

} // bindings
} // quartz