#include <quartz/core/parse.hpp>
#include <cppast/libclang_parser.hpp>
#include <cppast/visitor.hpp>
#include <cppast/cpp_entity_index.hpp>
#include <cppast/cpp_class.hpp>
#include <cppast/cpp_member_function.hpp>
#include <cppast/cpp_member_variable.hpp>
#include <quartz/core/helpers.hpp>
#include <fmt/format.h>
#include <fmt/base.h>
#include <algorithm>

namespace quartz
{

parsed_class parse(const std::filesystem::path& header)
{
    auto timer_start = start_timer();

    cppast::libclang_parser parser;
    cppast::cpp_entity_index index;
    cppast::libclang_compile_config config;
    config.set_flags(cppast::cpp_standard::cpp_20);

    auto file = parser.parse(index, header.string(), config);

    if (!file)
    {
        throw std::runtime_error(fmt::format("Failed to parse \"{}\"\n", header.string()));
    }

    parsed_class ret;
    ret.name = header.stem().string();
    ret.ns = get_namespace(header);

    cppast::visit(
        *file,
        [&ret](const cppast::cpp_entity& entity, cppast::visitor_info info) -> bool
        {
            // dont process twice
            if (info.event != cppast::visitor_info::container_entity_enter)
            {
                return true;
            }

            if (entity.kind() == cppast::cpp_entity_kind::class_t)
            {
                const auto& found_class = static_cast<const cppast::cpp_class&>(entity);

                if (found_class.name() == ret.name)
                {
                    for (const auto& base : found_class.bases())
                    {
                        ret.base_classes.push_back(base.name());
                    }

                    for (const auto& member : found_class)
                    {
                        auto kind = member.kind();

                        if (kind == cppast::cpp_entity_kind::constructor_t)
                        {
                            const auto& ctor = static_cast<const cppast::cpp_constructor&>(member);

                            parsed_constructor parsed_ctor;
                            parsed_ctor.name = ctor.name();

                            for (const auto& arg : ctor.parameters())
                            {
                                parsed_ctor.args.emplace_back(parsed_var(cppast::to_string(arg.type()), arg.name()));
                            }

                            ret.constructors.push_back(parsed_ctor);
                        }

                        if (kind == cppast::cpp_entity_kind::member_function_t)
                        {
                            const auto& mem_fn = static_cast<const cppast::cpp_constructor&>(member);

                            parsed_function parsed_fn;
                            parsed_fn.name = mem_fn.name();
                            // im not sure why `mem_fn.comment().value_or(..)` isn't working
                            parsed_fn.comment = mem_fn.comment().has_value() ? mem_fn.comment().value() : "";

                            for (const auto& arg : mem_fn.parameters())
                            {
                                parsed_fn.args.emplace_back(parsed_var(cppast::to_string(arg.type()), arg.name()));
                            }

                            ret.member_functions.push_back(parsed_fn);
                        }

                        if (kind == cppast::cpp_entity_kind::member_variable_t)
                        {
                            const auto& mem_var = static_cast<const cppast::cpp_member_variable&>(member);

                            parsed_var parsed_v;
                            parsed_v.type = cppast::to_string(mem_var.type());
                            parsed_v.name = mem_var.name();

                            ret.member_variables.push_back(parsed_v);
                        }
                    }

                    return true;
                }
            }

            return true;
        }
    );

    std::sort(ret.base_classes.begin(), ret.base_classes.end());

    std::sort(ret.member_functions.begin(), ret.member_functions.end(),
        [](const parsed_function& a, const parsed_function& b)
        {
            return a.name < b.name;
        }
    );

    std::sort(ret.member_variables.begin(), ret.member_variables.end(),
        [](const parsed_var& a, const parsed_var& b)
        {
            return a.name < b.name;
        }
    );

    fmt::print("Successfully parsed \"{}\" | took: {}s\n", ret.name, quartz::end_timer(timer_start));

    return ret;
}

} // namespace quartz