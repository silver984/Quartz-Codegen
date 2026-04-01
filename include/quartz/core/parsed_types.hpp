#pragma once
#include <string>
#include <vector>

namespace quartz
{

struct parsed_var
{
    std::string type;
    std::string name;
};

struct parsed_constructor
{
    std::string name;
    std::vector<parsed_var> args;
};

struct parsed_function
{
    std::string name;
    std::string comment;
    std::vector<parsed_var> args;
};

struct parsed_class
{
    std::string name;
    std::string ns; // namespace
    std::vector<std::string> base_classes;
    std::vector<parsed_constructor> constructors;
    std::vector<parsed_function> member_functions;
    std::vector<parsed_var> member_variables;
};

} // namespace quartz