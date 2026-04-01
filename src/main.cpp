#include <quartz/core/filesys.hpp>
#include <quartz/core/timer.hpp>
#include <cppast/libclang_parser.hpp>
#include <cppast/cpp_entity_index.hpp>
#include <cppast/cpp_class.hpp>
#include <cppast/visitor.hpp>
#include <fmt/format.h>
#include <filesystem>
#include <stdexcept>
#include <fstream>
#include <string>
#include <cctype>

// TODO: move these somewhere else

namespace quartz
{

std::string pascal_to_camel(const std::string& input)
{
    if (input.empty())
    {
        return input;
    }

    std::string result = input;
    result[0] = std::tolower(result[0]);
    return result;
}
} // namespace quartz

namespace quartz::bindings
{

void generate_header(const std::filesystem::path& header)
{
    auto start_timer = quartz::start_timer();

    std::filesystem::path output_path = std::filesystem::path("outputs") / std::filesystem::path("bindings") / header;
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
        // we remove "headers/" so it cannot be detected as the namespace
        auto fixed_path = std::filesystem::relative(header, "headers");
        std::string ns = fixed_path.has_parent_path() ? fixed_path.parent_path().string() : "";

        auto class_name = header.stem().string();
        auto class_name_camel = quartz::pascal_to_camel(class_name);
        std::string formatted_tmpl;

        if (ns.empty()) // no namespace
        {
            std::string tmpl = R"(namespace quartz
{{

struct {class}Bindings
{{
    {class}Bindings();
}};

}} // namespace quartz

// this variable's constructor queues bindings at static initialization time
static const quartz::{class}Bindings s_{class_camel}Bindings;)";

            formatted_tmpl = fmt::format(
                fmt::runtime(tmpl),
                fmt::arg("class", class_name),
                fmt::arg("class_camel", class_name_camel)
            );
        }
        else // namespace found, so add it
        {
            std::string tmpl = R"(namespace quartz::{namespace}
{{

struct {class}Bindings
{{
    {class}Bindings();
}};

}} // namespace quartz::{namespace}

// this variable's constructor queues bindings at static initialization time
static const quartz::{namespace}::{class}Bindings s_{class_camel}Bindings;)";

            formatted_tmpl = fmt::format(
                fmt::runtime(tmpl),
                fmt::arg("namespace", ns),
                fmt::arg("class", class_name),
                fmt::arg("class_camel", class_name_camel)
            );
        }

        created_file << formatted_tmpl;
    }
    catch (const std::ofstream::failure& e)
    {
        throw;
    }

    created_file.close();
    fmt::print("Successfully generated \"{}\" | took: {}s\n", output_path.string(), quartz::end_timer(start_timer));
}

void generate_impl(const std::filesystem::path& header)
{
    // TODO

    cppast::libclang_parser parser;
    cppast::cpp_entity_index index;
    cppast::libclang_compile_config config;
    config.set_flags(cppast::cpp_standard::cpp_20);

    auto file = parser.parse(index, header.string(), config);

    if (!file)
    {
        throw std::runtime_error(fmt::format("Failed to parse \"{}\"\n", header.string()));
    }

    cppast::visit(
        *file,
        [&header](const cppast::cpp_entity& entity, cppast::visitor_info info)
        {
            // dont process twice
            if (info.event != cppast::visitor_info::container_entity_enter)
            {
                return true;
            }

            if (entity.kind() == cppast::cpp_entity_kind::class_t)
            {
                const auto& found_class = static_cast<const cppast::cpp_class&>(entity);
                auto class_name = found_class.name();

                if (class_name == header.stem().string())
                {
                    fmt::print("found class: \"{}\"\n", class_name);

                    for (const auto& base : found_class.bases())
                    {
                        fmt::print("Base(s): \"{}\"\n", cppast::to_string(base.type()));
                    }
                }
            }

            return true;
        }
    );
}

} // namespace quartz::bindings

int main()
{
    std::filesystem::path outputs_folder = "outputs";

    try
    {
        // delete the output folder to get rid of old output
        std::filesystem::remove_all(outputs_folder);
    }
    catch (std::filesystem::filesystem_error& e)
    {
        fmt::print("Failed to delete the output folder (generated output may contain old output)\n");
    }

    std::filesystem::path bindings_folder = outputs_folder / "bindings";

    for (const auto& header : quartz::headers())
    {   
        try
        {
            quartz::bindings::generate_header(header);
        }
        catch (const std::runtime_error& re)
        {
            throw;
        }
    }

	return 0;
}