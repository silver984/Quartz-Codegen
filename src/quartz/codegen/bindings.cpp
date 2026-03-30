#include <quartz/codegen/bindings.hpp>
#include <fmt/format.h>

namespace quartz
{

std::string bindings_decl(const std::string& class_name, const std::string& ns)
{
    if (ns.empty())
    {
        const std::string tmpl = R"(#pragma once

namespace quartz
{{

class {class}Bindings
{{
public:
    {class}Bindings();
    ~{class}Bindings() = default;
}};

// queued bindings at static initialization time
static const {class}Bindings s_{class};

}} // quartz)";

        return fmt::format(fmt::runtime(tmpl),
                           fmt::arg("class", class_name));
    }

    const std::string namespaced_tmpl = R"(#pragma once

namespace quartz
{{
namespace {ns}
{{

class {class}Bindings
{{
public:
    {class}Bindings();
    ~{class}Bindings() = default;
}};

// queued bindings at static initialization time
static const {class}Bindings s_{class};

}} // {ns}
}} // quartz)";

    return fmt::format(fmt::runtime(namespaced_tmpl),
                       fmt::arg("ns", ns),
                       fmt::arg("class", class_name));
}

} // namespace quartz