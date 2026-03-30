#include <quartz/core/headers.hpp>

namespace quartz
{

std::vector<std::filesystem::path> headers()
{
    std::vector<std::filesystem::path> ret;
    std::filesystem::path base = "headers";

    for (const auto& entry : std::filesystem::recursive_directory_iterator(base))
    {
        ret.push_back(std::filesystem::relative(entry.path(), base));
    }

    return ret;
}

} // quartz