#include <quartz/codegen/bindings.hpp>
#include <quartz/core/helpers.hpp>
#include <quartz/core/filesys.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <algorithm>

namespace
{

std::string generate_ctors_str(const quartz::parsed_class& parsed)
{
    std::string str;
    bool is_ns_empty = parsed.ns.empty();
    std::string ns_right_alined = parsed.ns + "::";

    for (const auto& constructor : parsed.constructors)
    {
        std::string args;

        for (const auto& arg : constructor.args)
        {
            args += arg.type + ", ";
        }

        // remove the last ", " for this string
        quartz::remove_trailing_end(args, 2);

        str += fmt::format(
            "{MAYBE_NAMESPACE}{NAME}({ARGS})",
            fmt::arg("MAYBE_NAMESPACE", is_ns_empty ? "" : ns_right_alined),
            fmt::arg("NAME", constructor.name),
            fmt::arg("ARGS", args)
        ) + ", ";
    }
    
    // remove the last ", " for this string
    quartz::remove_trailing_end(str, 2);
    std::string implicit_ctor;

    if (str.empty())
    {
        implicit_ctor = fmt::format(
            "{MAYBE_NAMESPACE}{NAME}()",
            fmt::arg("MAYBE_NAMESPACE", is_ns_empty ? "" : ns_right_alined),
            fmt::arg("NAME", parsed.name)
        );
    }

    str = fmt::format(
        "sol::constructors<{CONSTRUCTORS}>()",
        fmt::arg("CONSTRUCTORS", str.empty() ? implicit_ctor : str)
    );

    return str;
}

// generates an empty string if `parsed.base_classes` are empty
std::string generate_bases_str(const quartz::parsed_class& parsed, size_t indent)
{
    if (parsed.base_classes.empty())
    {
        return {};
    }

    std::string indentation(indent, ' ');

    return fmt::format(
        ",\n{INDENT}sol::base_classes, sol::bases<{BASES}>()",
        fmt::arg("INDENT", indentation),
        fmt::arg("BASES", fmt::join(parsed.base_classes, ", "))
    );
}

std::string generate_member_vars_str(const quartz::parsed_class& parsed, size_t indent)
{
    std::string str;
    std::string indentation(indent, ' ');

    for (size_t i = 0; i < parsed.member_variables.size(); i++)
    {
        const auto& member = parsed.member_variables[i];

        str += fmt::format(
            ",{MAYBE_COMMENT}\n{INDENT}\"{SNAKE_CASE_MEMBER}\", &{MAYBE_NAMESPACE}{CLASS}::{MEMBER}{MAYBE_POST_COMMENT}",
            fmt::arg("MAYBE_COMMENT", i >= 1 ? " // " + parsed.member_variables[i - 1].type : ""),
            fmt::arg("INDENT", indentation),
            fmt::arg("SNAKE_CASE_MEMBER", quartz::camel_to_snake(quartz::remove_prefix(member.name, "m_"))),
            fmt::arg("MAYBE_NAMESPACE", parsed.ns.empty() ? "" : parsed.ns + "::"),
            fmt::arg("CLASS", parsed.name),
            fmt::arg("MEMBER", member.name),
            fmt::arg("MAYBE_POST_COMMENT", i == parsed.member_variables.size() - 1 ? " // " + member.type : "")
        );
    }

    return str;
}

// generates an empty string is `parsed.ns` is empty
std::string generate_namespace_table_str(const quartz::parsed_class& parsed)
{
    if (parsed.ns.empty())
    {
        return {};
    }

    return fmt::format(
        "\nsol::table ns = state[\"{NAMESPACE}\"].get_or_create<sol::table>();",
        fmt::arg("NAMESPACE", parsed.ns)
    );
}

std::string generate_usertype_str(const quartz::parsed_class& parsed)
{
    size_t indent = 4;

    return fmt::format(R"(auto& state = luaManager.luaState();{MAYBE_NAMESPACE_TABLE}
{STATE_OR_TABLE}.new_usertype<{MAYBE_NAMESPACE}{CLASS}>(
    "{CLASS}",
    {CONSTRUCTORS}{MAYBE_BASES}{MAYBE_MEMBER_VARIABLES}
);

sol::table usertype = {STATE_OR_TABLE}["{CLASS}"];)",
        fmt::arg("MAYBE_NAMESPACE_TABLE", generate_namespace_table_str(parsed)),
        fmt::arg("STATE_OR_TABLE", parsed.ns.empty() ? "state" : "ns"),
        fmt::arg("MAYBE_NAMESPACE", parsed.ns.empty() ? "" : parsed.ns + "::"),
        fmt::arg("CLASS", parsed.name),
        fmt::arg("CONSTRUCTORS", generate_ctors_str(parsed)),
        fmt::arg("MAYBE_BASES", generate_bases_str(parsed, indent)),
        fmt::arg("MAYBE_MEMBER_VARIABLES", generate_member_vars_str(parsed, indent))
    );
}

std::string generate_alloc_str(const quartz::parsed_class& parsed)
{
    std::string lambda_str;

    for (size_t i = 0; i < parsed.constructors.size(); i++)
    {
        const auto& ctor = parsed.constructors[i];
        std::string args;
        std::string args_forward;

        for (size_t j = 0; j < ctor.args.size(); j++)
        {
            const auto& arg = ctor.args[j];

            // if the constructor is declared like this: `name(void);`
            // it explicitly takes no arguments
            if (arg.type == "void" && arg.name.empty())
            {
                break;
            }

            // generates a numbered argument in case the argument's name is empty
            std::string generated_name =
                arg.name.empty()
                ? fmt::format("a{}", j)
                : arg.name;

            args += fmt::format(
                "{TYPE} {NAME}, ",
                fmt::arg("TYPE", arg.type),
                fmt::arg("NAME", generated_name)
            );

            args_forward += fmt::format(
                "{NAME}, ",
                fmt::arg("NAME", generated_name)
            );
        }

        // remove the last ", " in each of these strings
        quartz::remove_trailing_end(args, 2);
        quartz::remove_trailing_end(args_forward, 2);

        lambda_str += fmt::format(R"({MAYBE_NEWLINE}[]({MAYBE_ARGS}) -> {MAYBE_NAMESPACE}{CLASS}*
{{
    {MAYBE_NAMESPACE}{CLASS}* ptr = new(std::nothrow) {MAYBE_NAMESPACE}{CLASS}({MAYBE_ARGS_FORWARD});
    return ptr;
}})",
            // only generate a new line if were in the second iteration
            fmt::arg("MAYBE_NEWLINE", i >= 1 ? ",\n" : ""),
            fmt::arg("MAYBE_ARGS", args),
            fmt::arg("MAYBE_NAMESPACE", parsed.ns.empty() ? "" : parsed.ns + "::"),
            fmt::arg("CLASS", ctor.name),
            fmt::arg("MAYBE_ARGS_FORWARD", args_forward)
        );
    }

    // there was no constructor, therefore the lambda is empty
    // classes without explicit constructors have implicit constructors
    // so we still generate it
    if (lambda_str.empty())
    {
        lambda_str = fmt::format(R"([]() -> {MAYBE_NAMESPACE}{CLASS}*
{{
    {MAYBE_NAMESPACE}{CLASS}* ptr = new(std::nothrow) {MAYBE_NAMESPACE}{CLASS}();
    return ptr;
}})",
            fmt::arg("MAYBE_NAMESPACE", parsed.ns.empty() ? "" : parsed.ns + "::"),
            fmt::arg("CLASS", parsed.name)
        );
    }

    // indent by 8 if the there are more than one constructor
    // this is so it sits inside `sol::overload(...)`
    // indent by 4 if there's only one
    lambda_str = quartz::indent_lines(lambda_str, parsed.constructors.size() > 1 ? 8 : 4);
    std::string str;

    if (parsed.constructors.size() > 1) // constructor has overload
    {
        str = R"(usertype.set_function(
    "alloc",
    sol::overload(
{LAMBDA}
    )
);)";
    }
    else
    {
        str = R"(usertype.set_function(
    "alloc",
{LAMBDA}
);)";
    }

    str = fmt::format(
        fmt::runtime(str),
        fmt::arg("LAMBDA", lambda_str)
    );

    return str;
}

} // namespace <unnamed>

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

    std::string formatted_tmpl = fmt::format(
        R"(namespace quartz{MAYBE_NAMESPACE}
{{

struct {CLASS}Bindings
{{
    {CLASS}Bindings();
}};

}} // namespace quartz{MAYBE_NAMESPACE}

// this variable's constructor queues bindings at static initialization time
// this ensures that bindings are populated before running scripts
static const quartz{MAYBE_NAMESPACE}::{CLASS}Bindings {CLASS_CAMEL}Bindings;)",
        fmt::arg("MAYBE_NAMESPACE", parsed.ns.empty() ? "" : "::" + parsed.ns),
        fmt::arg("CLASS", parsed.name),
        fmt::arg("CLASS_CAMEL", quartz::pascal_to_camel(parsed.name))
    );

    created_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);

    try
    {
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

    if (parsed.ns.empty())
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

    std::string new_usertype = generate_usertype_str(parsed);
    new_usertype = quartz::indent_lines(new_usertype, 12);

    std::string alloc_str = generate_alloc_str(parsed);
    alloc_str = quartz::indent_lines(alloc_str, 12);

    /*
    std::string functions_str;
    
    for (const auto& [name, parsed_fn] : parsed.member_functions)
    {
        auto range = parsed.member_functions.equal_range(name);
        std::string fn_lambda_str;

        for (auto& it = range.first; it != range.second; ++it)
        {
            
        }
    }
    */

    std::string impl = fmt::format(R"(#include <quartz/bindings/{MAYBE_NAMESPACE_FOLDER}{CLASS}.hpp>
#include <quartz/modified/{MAYBE_NAMESPACE_FOLDER}{CLASS}.hpp>
#include <quartz/core/LuaManager.hpp>
#include <new>

namespace quartz{MAYBE_NAMESPACE_LEFT}
{{

{CLASS}Bindings::{CLASS}Bindings()
{{
    auto& luaManager = LuaManager::get();
    luaManager.queueBinding(
        [&luaManager]()
        {{
{NEW_USERTYPE}
            
            usertype.set_function(
                "fields",
                [](sol::this_state s, {MAYBE_NAMESPACE_RIGHT}{CLASS}* self) -> sol::table
                {{
                    sol::state_view lua(s);
                    
                    if (!self)
                    {{
                        return lua.create_table();
                    }}

                    auto modifiedSelf = static_cast<{CLASS}Modified*>(self);
                    if (modifiedSelf->m_fields)
                    {{
                        // this cast is required to access lua field storage
                        auto& luaFields = modifiedSelf->m_fields->m_luaFields;
                    
                        if (!luaFields.valid())
                        {{
                            // lazily create lua fields on first access
                            luaFields = lua.create_table();
                        }}
                        
                        return luaFields;
                    }}
                    
                    return lua.create_table();
                }}
            );
            
            // manual allocation exposed to lua
            // returns raw pointer
            // lua must call free()
{ALLOC}
            
            // manual deallocation for alloc()
            usertype.set_function(
                "free",
                []({MAYBE_NAMESPACE_RIGHT}{CLASS}* self)
                {{
                    delete self;
                }}
            );
        }}
    );
}}

}} // namespace quartz{MAYBE_NAMESPACE_LEFT})",
        fmt::arg("MAYBE_NAMESPACE_FOLDER", parsed.ns.empty() ? "" : parsed.ns + "/"),
        fmt::arg("CLASS", parsed.name),
        fmt::arg("MAYBE_NAMESPACE_LEFT", parsed.ns.empty() ? "" : "::" + parsed.ns),
        fmt::arg("NEW_USERTYPE", new_usertype),
        fmt::arg("MAYBE_NAMESPACE_RIGHT", parsed.ns.empty() ? "" : parsed.ns + "::"),
        fmt::arg("ALLOC", alloc_str)
    );

    created_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);

    try
    {
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