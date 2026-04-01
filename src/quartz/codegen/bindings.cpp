#include <quartz/codegen/bindings.hpp>
#include <quartz/core/helpers.hpp>
#include <quartz/core/filesys.hpp>
#include <fmt/format.h>
#include <filesystem>
#include <fstream>

namespace quartz::bindings
{

void generate_header(const quartz::parsed_class& parsed)
{
    auto start_timer = quartz::start_timer();

    std::filesystem::path output_path = std::filesystem::path("outputs") / "headers" / "bindings";

    if (parsed.ns.empty())
    {
        output_path = output_path / parsed.name;
    }
    else
    {
        output_path = output_path / parsed.ns / parsed.name;
    }

    output_path.replace_extension(".hpp");
    std::ofstream created_file;

    try
    {
        quartz::try_create_file(created_file, output_path);
    }
    catch (const std::runtime_error& re)
    {
        throw;
    }

    created_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);

    try
    {
        std::string formatted_tmpl = fmt::format(
            R"(namespace quartz{maybe_ns}
{{

struct {class}Bindings
{{
    {class}Bindings();
}};

}} // namespace quartz{maybe_ns}

// this variable's constructor queues bindings at static initialization time
static const quartz{maybe_ns}::{class}Bindings {class_camel}Bindings;)",
            fmt::arg("maybe_ns", parsed.ns.empty() ? "" : "::" + parsed.ns),
            fmt::arg("class", parsed.name),
            fmt::arg("class_camel", quartz::pascal_to_camel(parsed.name))
        );

        created_file << formatted_tmpl;
    }
    catch (const std::ofstream::failure& e)
    {
        throw;
    }

    created_file.close();
    fmt::print("Successfully generated \"{}\" | took: {}s\n", output_path.string(), quartz::end_timer(start_timer));
}

void generate_impl(const quartz::parsed_class& parsed)
{
    auto start_timer = quartz::start_timer();
    std::filesystem::path output_path = std::filesystem::path("outputs") / "impl" / "bindings";
    bool is_parsed_ns_empty = parsed.ns.empty();

    if (is_parsed_ns_empty)
    {
        output_path = output_path / parsed.name;
    }
    else
    {
        output_path = output_path / parsed.ns / parsed.name;
    }

    output_path.replace_extension(".cpp");
    std::ofstream created_file;

    try
    {
        quartz::try_create_file(created_file, output_path);
    }
    catch (const std::runtime_error& re)
    {
        throw;
    }

    created_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);

    std::string constructors_str;

    for (const auto& constructor : parsed.constructors)
    {
        std::string args;

        for (const auto& arg : constructor.args)
        {
            args += arg.type + ", ";
        }

        quartz::remove_trailing_end(args, 2);

        constructors_str += fmt::format(
            "{ns}{name}({args})",
            fmt::arg("ns", is_parsed_ns_empty ? "" : parsed.ns + "::"),
            fmt::arg("name", constructor.name),
            fmt::arg("args", args)
        ) + ", ";
    }

    if (constructors_str.empty())
    {
        constructors_str = "sol::no_constructor";
    }
    else
    {
        quartz::remove_trailing_end(constructors_str, 2);
        constructors_str = fmt::format("sol::constructors<{}>()", constructors_str);
    }

    std::string base_classes_str;

    for (const auto& base_class : parsed.base_classes)
    {
        base_classes_str += base_class + ", ";
    }

    if (!base_classes_str.empty())
    {
        quartz::remove_trailing_end(base_classes_str, 2);
        base_classes_str = fmt::format(",\n    sol::base_classes, sol::bases<{}>()", base_classes_str);
    }

    std::string member_variables_str;

    for (const auto& mem_v : parsed.member_variables)
    {
        member_variables_str += fmt::format(
            ",\n    \"{snake_case_mem_v}\", &{maybe_ns}{class}::{mem_v}",
            fmt::arg("snake_case_mem_v", quartz::camel_to_snake(quartz::remove_prefix(mem_v.name, "m_"))),
            fmt::arg("maybe_ns", is_parsed_ns_empty ? "" : parsed.ns + "::"),
            fmt::arg("class", parsed.name),
            fmt::arg("mem_v", mem_v.name)
        );

    }

    std::string ns_table;

    if (!is_parsed_ns_empty)
    {
        ns_table = fmt::format("\n\nsol::table ns = state[\"{ns}\"].get_or_create<sol::table>();", fmt::arg("ns", parsed.ns));
    }

    std::string new_usertype = fmt::format(R"(auto& state = luaManager.luaState();{maybe_ns_table}

{state_or_table}.new_usertype<{maybe_ns}{class}>(
    "{class}",
    {constructors}{maybe_base_classes}{maybe_member_variables}
);

sol::table usertype = {state_or_table}["{class}"];)",
        fmt::arg("maybe_ns_table", ns_table),
        fmt::arg("state_or_table", is_parsed_ns_empty ? "state" : "ns"),
        fmt::arg("maybe_ns", is_parsed_ns_empty ? "" : parsed.ns + "::"),
        fmt::arg("class", parsed.name),
        fmt::arg("constructors", constructors_str),
        fmt::arg("maybe_base_classes", base_classes_str),
        fmt::arg("maybe_member_variables", member_variables_str)
    );

    new_usertype = quartz::indent_lines(new_usertype, 12);

    try
    {
        std::string impl = fmt::format(R"(#include <quartz/bindings/{maybe_ns_folder}{class}.hpp>
#include <quartz/hooks/{maybe_ns_folder}{class}.hpp>
#include <quartz/core/LuaManager.hpp>
#include <new>

namespace quartz{maybe_ns}
{{

{class}Bindings::{class}Bindings()
{{
    auto& luaManager = quartz::LuaManager::get();
    luaManager.queueBinding(
        [&luaManager]()
        {{
{new_usertype}
        }}
    );
}}

}} // namespace quartz{maybe_ns})",
            fmt::arg("maybe_ns_folder", is_parsed_ns_empty ? "" : parsed.ns + "/"),
            fmt::arg("class", parsed.name),
            fmt::arg("maybe_ns", is_parsed_ns_empty ? "" : "::" + parsed.ns),
            fmt::arg("new_usertype", new_usertype)
        );

        created_file << impl;
    }
    catch (const std::ofstream::failure& e)
    {
        throw;
    }

    created_file.close();
    fmt::print("Successfully generated \"{}\" | took: {}s\n", output_path.string(), quartz::end_timer(start_timer));
}

} // namespace quartz::bindings