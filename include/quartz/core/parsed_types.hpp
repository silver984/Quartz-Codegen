#pragma once
#include <string>
#include <vector>
#include <map>

namespace quartz {

struct parsed_var {
    std::string type;
    std::string name;
};

struct parsed_constructor {
    std::string name;
    std::vector<parsed_var> args;
};

struct parsed_function {
    std::string return_type;
    std::vector<std::string> modifiers;
    std::string comment;
    bool is_virtual;
    bool is_static;
    bool is_out_of_line;
    std::vector<parsed_var> args;
};

struct parsed_class {
    std::string name;
    std::string ns; // namespace
    std::vector<std::string> base_classes;
    std::vector<parsed_constructor> constructors;
    std::multimap<std::string, parsed_function> member_functions; // this is multimap for function overloads
    std::vector<parsed_var> member_variables;
};

} // namespace quartz