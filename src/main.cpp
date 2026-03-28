#include <string>
#include <vector>
#include <iostream>
#include <unordered_set>
#include <sstream>

namespace quartz
{

struct arg
{
    std::string type;
    std::string name;
};

struct function_decl
{
    std::string full_string;
    std::vector<std::string> pre_modifiers;
    std::vector<std::string> post_modifiers;
    std::string return_type;
    std::string name;
    std::vector<arg> args;
};

const std::unordered_set<std::string> pre_modifier_set = {
    "inline",
    "virtual",
    "constexpr",
    "static",
    "extern",
    "friend",
    "explicit",
    "consteval",
    "constinit",
    "mutable"
};

const std::unordered_set<std::string> post_modifier_set = {
    "const",
    "noexcept",
    "override",
    "final",
    "volatile",
    "&",
    "&&"
};

std::string trim_string(const std::string& str)
{
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");

    if (start == std::string::npos)
    {
        return "";
    }

    return str.substr(start, end - start + 1);
}

function_decl parse_function(const std::string& str)
{
    function_decl parsed;
    parsed.full_string = str;

    std::string copy = str;
    copy.erase(copy.find(';'));

    size_t open_paren = copy.find('(');
    size_t close_paren = copy.find(')');

    std::string before = trim_string(copy.substr(0, open_paren));
    std::string args_str = copy.substr(open_paren + 1, close_paren - open_paren - 1);
    std::string after = trim_string(copy.substr(close_paren + 1));

    std::stringstream ss(before);
    std::vector<std::string> tokens;
    std::string tok;

    while (ss >> tok)
    {
        tokens.push_back(tok);
    }

    // detect pre modifiers
    size_t i = 0;
    while (i < tokens.size() && pre_modifier_set.contains(tokens[i]))
    {
        parsed.pre_modifiers.push_back(tokens[i]);
        i++;
    }

    // last token is function name
    parsed.name = tokens.back();
    tokens.pop_back();

    // remaining tokens form return type
    parsed.return_type = "";
    
    for (size_t j = i; j < tokens.size(); ++j)
    {
        parsed.return_type += tokens[j] + " ";
    }

    parsed.return_type = trim_string(parsed.return_type);

    // parse args
    std::stringstream arg_ss(args_str);
    std::string arg_token;

    while (std::getline(arg_ss, arg_token, ','))
    {
        arg_token = trim_string(arg_token);

        if (arg_token.empty())
        {
            continue;
        }

        std::stringstream a(arg_token);
        std::vector<std::string> parts;
        std::string p;

        while (a >> p)
        {
            parts.push_back(p);
        }

        if (parts.size() >= 2)
        {
            arg parsed_arg;
            parsed_arg.name = parts.back();
            parts.pop_back();

            parsed_arg.type = "";

            for (auto& x : parts)
            {
                parsed_arg.type += x + " ";
            }

            parsed_arg.type = trim_string(parsed_arg.type);

            parsed.args.push_back(parsed_arg);
        }
    }

    // parse post modifiers
    std::stringstream post_ss(after);
    while (post_ss >> tok)
    {
        if (post_modifier_set.contains(tok))
        {
            parsed.post_modifiers.push_back(tok);
        }
    }

    return parsed;
}

bool is_valid_line(const std::string& str)
{
    return str.ends_with(';');
}

} // quartz

int main()
{
    // added tabs for testing if trimming works
    std::string test = "        constexpr int func(int p0, bool p1, float p2) const noexcept override;         ";
    
    auto trimmed = quartz::trim_string(test);

    if (!quartz::is_valid_line(trimmed))
    {
        std::cout << "not a valid line\n";
        return 0;
    }

    auto parsed = quartz::parse_function(trimmed);
    std::cout << "declaration: \"" << parsed.full_string << "\"\n";

    for (const auto& pre_mod : parsed.pre_modifiers)
    {
        std::cout << "pre_modifier(s): \"" << pre_mod << "\"\n";
    }

    std::cout << "return type: \"" << parsed.return_type << "\"\n";
    std::cout << "name: \"" << parsed.name << "\"\n";

    for (const auto& a : parsed.args)
    {
        std::cout << "arg: type=\"" << a.type << "\" name=\"" << a.name << "\"\n";
    }

    for (const auto& post_mod : parsed.post_modifiers)
    {
        std::cout << "post_modifier(s): \"" << post_mod << "\"\n";
    }

    return 0;
}
