#include <quartz/core/filesys.hpp>
#include <fmt/base.h>

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

bool try_create_dir(const std::filesystem::path& path)
{
    std::error_code ec;
    std::filesystem::create_directories(path, ec);

    if (ec)
    {
        fmt::print("Failed to create directory: \"{}\" | message: {}\n", path.string(), ec.message());
        return false;
    }

    return true;
}

} // quartz