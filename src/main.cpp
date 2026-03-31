#include <quartz/codegen/bindings.hpp>
#include <quartz/core/filesys.hpp>
#include <quartz/core/timer.hpp>
#include <cppast/libclang_parser.hpp>
#include <cppast/cpp_entity_index.hpp>
#include <cppast/cpp_file.hpp>
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
        // generate bindings

        { // headers
            auto timer_begin = quartz::start_timer();

            std::filesystem::path output_path = bindings_folder / "headers" / header;
            output_path.replace_extension(".hpp");

            if (!quartz::try_create_dir(output_path.parent_path()))
            {
                continue;
            }

            std::ofstream created_file(output_path);

            if (!created_file)
            {
                fmt::print("Failed to create: \"{}\"\n", output_path.string());
                continue;
            }

            created_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);

            try
            {
                created_file << quartz::bindings::make_hpp(header.stem().string(), header.has_parent_path() ? header.parent_path().string() : "");
                fmt::print("Successfully wrote: \"{}\" | took {}s\n",
                           output_path.string(), quartz::end_timer(timer_begin));
            }
            catch (const std::ofstream::failure& e)
            {
                fmt::print("Failed to write: \"{}\" | what: {}\n",
                           output_path.string(), e.what());
                continue;
            }

            created_file.close();
        }
    }

	return 0;
}