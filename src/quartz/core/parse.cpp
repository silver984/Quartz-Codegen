#include <quartz/core/parse.hpp>
#include <cppast/libclang_parser.hpp>
#include <cppast/visitor.hpp>
#include <cppast/cpp_entity_index.hpp>
#include <cppast/cpp_class.hpp>
#include <cppast/cpp_member_function.hpp>
#include <cppast/cpp_member_variable.hpp>
#include <cppast/cpp_array_type.hpp>
#include <cppast/cpp_type.hpp>
#include <quartz/core/helpers.hpp>
#include <fmt/format.h>
#include <fmt/base.h>
#include <algorithm>

namespace {

std::string type_to_string(const cppast::cpp_type& type) {
    switch (type.kind()) {
    case cppast::cpp_type_kind::builtin_t: // fall-through
    case cppast::cpp_type_kind::user_defined_t: // fall-through
    case cppast::cpp_type_kind::auto_t: // fall-through
    case cppast::cpp_type_kind::decltype_t: // fall-through
    case cppast::cpp_type_kind::decltype_auto_t: // fall-through
    case cppast::cpp_type_kind::template_parameter_t: // fall-through
    case cppast::cpp_type_kind::template_instantiation_t:
        return cppast::to_string(type);

    case cppast::cpp_type_kind::reference_t: {
        auto& ref = static_cast<const cppast::cpp_reference_type&>(type);
        return type_to_string(ref.referee()) + "&";
    }

    case cppast::cpp_type_kind::pointer_t: {
        auto& ptr = static_cast<const cppast::cpp_pointer_type&>(type);
        return type_to_string(ptr.pointee()) + "*";
    }

    case cppast::cpp_type_kind::array_t: {
        auto& arr = static_cast<const cppast::cpp_array_type&>(type);
        return type_to_string(arr.value_type()) + "[]";
    }

    case cppast::cpp_type_kind::cv_qualified_t: {
        auto& cv = static_cast<const cppast::cpp_cv_qualified_type&>(type);
        std::string base = type_to_string(cv.type());
        std::string qualifier;

        if (cv.cv_qualifier() & cppast::cpp_cv::cpp_cv_const) {
            qualifier += " const";
        }

        if (cv.cv_qualifier() & cppast::cpp_cv::cpp_cv_volatile) {
            qualifier += " volatile";
        }

        return base + qualifier;
    }

    default:
        return cppast::to_string(type);
    }
}

} // namespace <unnamed>

namespace quartz {

parsed_class parse(const std::filesystem::path& header) {
    auto timer_start = start_timer();

    cppast::libclang_parser parser;
    cppast::cpp_entity_index index;
    cppast::libclang_compile_config config;
    config.set_flags(cppast::cpp_standard::cpp_20);

    auto file = parser.parse(index, header.string(), config);

    if (!file) {
        throw std::runtime_error(fmt::format("Failed to parse \"{}\"\n", header.string()));
    }

    parsed_class ret;
    ret.name = header.stem().string();
    ret.ns = get_namespace(header);

    cppast::visit( *file,
        [&ret](const cppast::cpp_entity& entity, cppast::visitor_info info) -> bool {
            // dont process twice
            if (info.event != cppast::visitor_info::container_entity_enter) {
                return true;
            }

            if (entity.kind() == cppast::cpp_entity_kind::class_t) {
                const auto& found_class = static_cast<const cppast::cpp_class&>(entity);

                if (found_class.name() == ret.name) {
                    for (const auto& base : found_class.bases()) {
                        ret.base_classes.push_back(base.name());
                    }

                    for (const auto& member : found_class) {
                        auto kind = member.kind();

                        if (kind == cppast::cpp_entity_kind::constructor_t) {
                            const auto& ctor = static_cast<const cppast::cpp_constructor&>(member);
                            parsed_constructor parsed_ctor;
                            parsed_ctor.name = ctor.name();

                            for (const auto& arg : ctor.parameters()) {
                                parsed_ctor.args.emplace_back(parsed_var(type_to_string(arg.type()), arg.name()));
                            }

                            ret.constructors.push_back(parsed_ctor);
                        }

                        // static functions
                        if (kind == cppast::cpp_entity_kind::function_t) {
                            const auto& mem_fn = static_cast<const cppast::cpp_function&>(member);
                            parsed_function parsed_fn;
                            parsed_fn.is_virtual = false;
                            parsed_fn.is_static = true;
                            parsed_fn.is_const_qualifier = false;
                            parsed_fn.is_volatile_qualifier = false;
                            parsed_fn.comment = mem_fn.comment().has_value() ? mem_fn.comment().value() : "";
                            // we only find one instance of the substring
                            // no need to specify platforms
                            parsed_fn.is_out_of_line = parsed_fn.comment.find("Out of line") != std::string::npos;
                            parsed_fn.return_type = type_to_string(mem_fn.return_type());
                            
                            for (const auto& arg : mem_fn.parameters()) {
                                parsed_fn.args.emplace_back(parsed_var(type_to_string(arg.type()), arg.name()));
                            }
                            
                            ret.member_functions.insert({ mem_fn.name(), parsed_fn });
                        }

                        // member functions
                        if (kind == cppast::cpp_entity_kind::member_function_t) {
                            const auto& mem_fn = static_cast<const cppast::cpp_member_function&>(member);
                            parsed_function parsed_fn;
                            parsed_fn.is_virtual = mem_fn.is_virtual();
                            parsed_fn.is_static = false;
                            parsed_fn.is_const_qualifier = mem_fn.cv_qualifier() & cppast::cpp_cv::cpp_cv_const;
                            parsed_fn.is_volatile_qualifier = mem_fn.cv_qualifier() & cppast::cpp_cv::cpp_cv_volatile;
                            parsed_fn.comment = mem_fn.comment().has_value() ? mem_fn.comment().value() : "";
                            // we only find one instance of the substring
                            // no need to specify platforms
                            parsed_fn.is_out_of_line = parsed_fn.comment.find("Out of line") != std::string::npos;
                            parsed_fn.return_type = type_to_string(mem_fn.return_type());
                            
                            for (const auto& arg : mem_fn.parameters()) {
                                parsed_fn.args.emplace_back(parsed_var(type_to_string(arg.type()), arg.name()));
                            }
                            
                            ret.member_functions.insert({ mem_fn.name(), parsed_fn });
                        }

                        if (kind == cppast::cpp_entity_kind::member_variable_t) {
                            const auto& mem_var = static_cast<const cppast::cpp_member_variable&>(member);
                            parsed_var parsed_v;
                            parsed_v.type = type_to_string(mem_var.type());
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

    // sort them out alpabetically
    std::sort(ret.base_classes.begin(), ret.base_classes.end());
    std::sort(ret.member_variables.begin(), ret.member_variables.end(),
        [](const parsed_var& a, const parsed_var& b) {
            return a.name < b.name;
        });

    fmt::print("Successfully parsed \"{}\" | took: {}s\n", ret.name, quartz::end_timer(timer_start));

    return ret;
}

} // namespace quartz