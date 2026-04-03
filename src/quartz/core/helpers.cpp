#include <quartz/core/helpers.hpp>
#include <sstream>

namespace quartz
{

std::string pascal_to_camel(const std::string& str)
{
    if (str.empty())
    {
        return str;
    }

    std::string result = str;
    result[0] = std::tolower(result[0]);
    return result;
}

std::string camel_to_snake(const std::string& str)
{
    std::string result;

    for (size_t i = 0; i < str.size(); ++i)
    {
        char c = str[i];

        if (std::isupper(static_cast<unsigned char>(c)))
        {
            if (i != 0 && !std::isupper(static_cast<unsigned char>(str[i - 1])))
            {
                result += '_';
            }
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        else
        {
            result += c;
        }
    }

    return result;
}

std::string remove_prefix(const std::string& str, const std::string& prefix)
{
    if (str.rfind(prefix, 0) == 0)
    {
        return str.substr(prefix.length());
    }

    return str;
}

std::string get_namespace(const std::filesystem::path& header)
{
    auto fixed_path = std::filesystem::relative(header, "headers");
    return fixed_path.has_parent_path() ? fixed_path.parent_path().string() : "";
}

std::string indent_lines(const std::string& str, size_t spaces)
{
    std::string indent(spaces, ' ');
    std::stringstream input(str);
    std::stringstream output;
    std::string line;

    while (std::getline(input, line))
    {
        if (!line.empty())
        {
            output << indent << line;
        }

        output << "\n";
    }
    
    auto output_str = output.str();
    remove_trailing_end(output_str, 1);
    return output_str;
}

double end_timer(const std::chrono::steady_clock::time_point& start)
{
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}

void remove_trailing_end(std::string& str, size_t count)
{
    if (!str.empty())
    {
        str.erase(str.size() - count);
    }
}

} // namespace quartz