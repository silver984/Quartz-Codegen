#include <quartz/core/filesys.hpp>
#include <fmt/format.h>
#include <stdexcept>

namespace quartz
{

std::vector<std::filesystem::path> headers()
{
    std::vector<std::filesystem::path> ret;

    for (const auto& entry : std::filesystem::recursive_directory_iterator("headers"))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        ret.push_back(entry.path());
    }

    return ret;
}

void try_create_dir(const std::filesystem::path& path)
{
    std::error_code ec;
    std::filesystem::create_directories(path, ec);

    if (ec)
    {
        throw std::runtime_error(fmt::format("Failed to create directory: \"{}\" | error code message: {}\n", path.string(), ec.message()));
    }
}

void try_create_file(std::ofstream& stream, const std::filesystem::path& path)
{
    try
    {
        try_create_dir(path.parent_path());
    }
    catch (const std::runtime_error& re)
    {
        throw;
    }

    stream.open(path, std::ios::out | std::ios::trunc);

    if (!stream.is_open())
    {
        throw std::runtime_error(fmt::format("Failed to create: \"{}\"\n", path.string()));
    }
}

} // quartz