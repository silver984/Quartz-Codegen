#include <quartz/codegen/bindings.hpp>
#include <quartz/core/filesys.hpp>
#include <quartz/core/parse.hpp>
#include <fmt/base.h>
#include <filesystem>
#include <stdexcept>

int main()
{
    try
    {
        // delete the output folder to get rid of old output
        std::filesystem::remove_all("outputs");
    }
    catch (std::filesystem::filesystem_error& e)
    {
        fmt::print("Failed to delete the output folder (generated output may contain old output) | what: {}\n", e.what());
    }

    for (const auto& header : quartz::headers())
    {   
        quartz::parsed_class parsed;

        try
        {
            parsed = quartz::parse(header);
        }
        catch (const std::runtime_error& re)
        {
            throw;
        }

        try
        {
            quartz::bindings::generate_header(parsed);
        }
        catch (const std::runtime_error& re)
        {
            throw;
        }

        quartz::bindings::generate_impl(parsed);
    }

	return 0;
}