#include <quartz/codegen/modified.hpp>
#include <quartz/core/helpers.hpp>
#include <quartz/core/filesys.hpp>
#include <fmt/format.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <cstdint>

namespace quartz::modified {

void generate_header(const quartz::parsed_class& parsed) {
    auto start_timer = quartz::start_timer();

    std::filesystem::path output_path = std::filesystem::path("outputs") / "headers" / "modified";

    if (parsed.ns.empty()) {
        output_path = output_path / parsed.name;
    } else {
        output_path = output_path / parsed.ns / parsed.name;
    }

    output_path.replace_extension(".hpp");

    std::ofstream created_file;

    try {
        quartz::try_create_file(created_file, output_path);
    } catch (const std::runtime_error& re) {
        throw;

    }
    std::string fn_declarations_str;
    size_t iterations = 0;
    for (auto it = parsed.member_functions.begin(); it != parsed.member_functions.end();) {
        const auto& fn_name = it->first;
        auto range = parsed.member_functions.equal_range(fn_name);
        size_t count = 0;

        for (auto jt = range.first; jt != range.second; ++jt) {
            count++;
            const auto& fn = jt->second;

            if (fn.is_out_of_line) {
                continue;
            }

            iterations++;

            std::string decl_str;

            decl_str += fmt::format("{MAYBE_NEWLINE}// @lua \"{MAYBE_NAMESPACE_LUA}{CLASS}{COLON_OR_DOT}{NAME}{MAYBE_INDEX}\"\n",
                fmt::arg("MAYBE_NEWLINE", iterations > 1 ? "\n" : ""),
                fmt::arg("MAYBE_NAMESPACE_LUA", parsed.ns.empty() ? "" : parsed.ns + "."),
                fmt::arg("CLASS", parsed.name),
                fmt::arg("COLON_OR_DOT", fn.is_static ? "." : ":"),
                fmt::arg("NAME", quartz::camel_to_snake(fn_name)),
                fmt::arg("MAYBE_INDEX", count > 1 ? fmt::format("@{}", count) : ""));

            if (fn.is_static) {
                decl_str += "static ";
            }

            std::string args_str;

            for (size_t i = 0; i < fn.args.size(); ++i) {
                const auto& arg = fn.args[i];
                bool is_arg_name_empty = arg.name.empty();

                // if the function is declared like this: `return_type name(void);`
                // it explicitly takes no arguments
                if (arg.type == "void" && is_arg_name_empty) {
                    break;
                }

                // generates a numbered argument in case the argument's name is empty
                std::string generated_name = is_arg_name_empty
                    ? fmt::format("a{}", i)
                    : arg.name;

                args_str += fmt::format("{TYPE} {NAME}, ",
                    fmt::arg("TYPE", arg.type),
                    fmt::arg("NAME", generated_name));
            }

            // remove the last ", "
            quartz::remove_trailing_end(args_str, 2);


            decl_str += fmt::format("{RETURN_TYPE} {NAME}({ARGS}){MAYBE_CONST}{MAYBE_VOLATILE}{MAYBE_OVERRIDE};\n",
                fmt::arg("RETURN_TYPE", fn.return_type),
                fmt::arg("NAME", fn_name),
                fmt::arg("ARGS", args_str),
                fmt::arg("MAYBE_CONST", fn.is_const_qualifier ? " const" : ""),
                fmt::arg("MAYBE_VOLATILE", fn.is_volatile_qualifier ? " volatile" : ""),
                fmt::arg("MAYBE_OVERRIDE", fn.is_virtual ? " override" : ""));

            fn_declarations_str += decl_str;
        }

        it = range.second; // skip processed group
    }

    fn_declarations_str = quartz::indent_lines(fn_declarations_str, 4);

    std::string header = fmt::format(R"(#pragma once
#include <Geode/modify/{CLASS}.hpp>
#include <quartz/core/LuaFields.hpp>

namespace quartz{MAYBE_NAMESPACE_LEFT} {{

struct {CLASS}Modified : geode::Modify<{CLASS}Modified, {MAYBE_NAMESPACE_RIGHT}{CLASS}>, quartz::LuaFields {{
{DECLARED_FUNCTIONS}
}}

}} // namespace quartz{MAYBE_NAMESPACE_LEFT})",
fmt::arg("MAYBE_NAMESPACE_LEFT", quartz::add_scope_qualifier(parsed.ns, quartz::scope_position::left)),
fmt::arg("CLASS", parsed.name),
fmt::arg("MAYBE_NAMESPACE_RIGHT", quartz::add_scope_qualifier(parsed.ns, quartz::scope_position::right)),
fmt::arg("DECLARED_FUNCTIONS", fn_declarations_str));

    created_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);

    try {
        created_file << header;
    } catch (const std::ofstream::failure& e) {
        throw;
    }

    created_file.close();

    fmt::print("Successfully generated \"{}\" | took: {}s\n", output_path.string(), quartz::end_timer(start_timer));
}

} // namespace quartz::modified