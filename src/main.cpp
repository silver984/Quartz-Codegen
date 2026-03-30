#include <quartz/codegen/bindings.hpp>
#include <quartz/core/headers.hpp>
#include <quartz/core/timer.hpp>
#include <fstream>
#include <filesystem>
#include <fmt/base.h>

int main()
{
    std::filesystem::path outputs_folder = "outputs";

    // delete the output folder to get rid of old output
    try
    {
        std::filesystem::remove_all(outputs_folder);
    }
    catch (std::filesystem::filesystem_error& e)
    {
        fmt::print("Failed to delete the output folder (generated output may contain old output)\n");
    }

    std::filesystem::path bindings_folder = outputs_folder / "bindings";

    for (const auto& header : quartz::headers())
    {   
        // generate bindings headers

        auto timer_begin = quartz::start_timer();

        std::filesystem::path output_path = bindings_folder / "headers" / header;
        output_path.replace_extension(".hpp");

        // try creating directories
        auto output_parent_path = output_path.parent_path();
        std::error_code ec;
        std::filesystem::create_directories(output_parent_path, ec);

        if (ec)
        {
            fmt::print("Failed to create directory: \"{}\" | message: {}\n", output_parent_path.string(), ec.message());
            continue;
        }

        std::ofstream created_file(output_path);

        if (!created_file)
        {
            fmt::print("Failed to create bindings header: \"{}\"\n", output_path.string());
            continue;
        }

        // enable exceptions for write failures
        created_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);

        try
        {
            created_file << quartz::bindings_decl(header.stem().string(), header.has_parent_path() ? header.parent_path().string() : "");
            auto timer_end = quartz::end_timer(timer_begin);
            fmt::print("Generated bindings header successfully: \"{}\" | took {}s\n", output_path.string(), timer_end);
        }
        catch (const std::ofstream::failure& e)
        {
            fmt::print("Failed to write bindings header: \"{}\" | what: {}\n", output_path.string(), e.what());
            continue;
        }

        created_file.close();
    }

	return 0;
}