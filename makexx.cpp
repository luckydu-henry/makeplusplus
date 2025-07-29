#include "tinyxml2/tinyxml2.h"
#include <iostream>
#include <string>
#include <random>
#include <format>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "cpod.hpp"
#include "tinyxml2/tinyxml2.h"
#include "makeplusplus.hpp"
#include "cpod.hpp"


static constexpr std::string_view g_hello_message =
R"(---------------------------------------------------------------------------------------------------------------------
Hello, welcome to use makeplusplus!
This software allows you to create visual studio solution project and makefile project with 'C++' code!
The software itself is also written in C++ and that results in its high performance.
This software is especially for those developers who want to have a lightweight portable building system
with a good speed, if you are building an application like a video game or an editor, you are going to love this,
however if your goal is to develop a third-party library and wants to include dependencies or some high-end features
easily, you might still want to use CMake.
You can use -h or --help command for more usages.
To create a makeplusplus project, just use -gp and you are ready to go.
---------------------------------------------------------------------------------------------------------------------)";

static constexpr std::string_view g_help_message =
R"(---------------------------------------------------------------------------------------------------------------------
-h/--help          : This command.
-gh                : Generate only platform dependent header with makeplusplus project structure.
-gp <project-name> : Generate complete project with makeplusplus project structure.
---------------------------------------------------------------------------------------------------------------------)";

static void generate_header() {
    std::ofstream header("makexx.generated.hpp");
    header << makexx::put_header_archive_to_buffer({
        {"MXX_PROJECT_PATH", std::format("\"{:s}/\"", std::filesystem::current_path().parent_path().generic_string())}
    });
    header.close();
    std::cout << "Header generated!\n";
}

static void generate_project(int argc, char** argv) {
    if (argc == 2) {
        std::cout << "Error, must have project name argument!\n";
        return;
    }
    std::string_view project_name = argv[2];
    if (!std::filesystem::exists("makexx.generated.hpp")) {
        generate_header();
    }
    std::ofstream project(std::format("../{:s}.make.hpp", project_name));
    project << "// Makeplusplus project!\n";
    project << std::format("#include \"{:s}/makexx.generated.hpp\"\n\n", std::filesystem::current_path().filename().generic_string());
    project << "PROJECT_NAME = \"\";\n";
    project << "PROJECT_TARGETS = {};\n";
    project << "PROJECT_CONFIGURATIONS = {};\n";
    project.close();
    std::cout << std::format("Poject descriptor \"{:s}.make.hpp\" has been written to \"{:s}\"\n", project_name, std::filesystem::current_path().relative_path().generic_string());

    std::filesystem::create_directory("bin");
    std::filesystem::create_directory("int");
    std::filesystem::create_directory("projects");
    std::cout << "Platform project folder, binary folder, itermediate folder creation complete!\n";
}


int main(int argc, char** argv) {
    
    using namespace std::string_view_literals;
    if (argc == 1) {
        std::cout << g_hello_message << std::endl;
        return 0;
    }
    // Generate project
    if      ("-gh"sv    == argv[1])    { generate_header(); }
    else if ("-gp"sv    == argv[1])    { generate_project(argc, argv); }
    else if ("-h"sv     == argv[1]  ||
             "--help"sv == argv[1])    {
        std::cout << g_help_message << std::endl;
    }

    return 0;
    
    // makexx::visual_studio_project vssln("SampleProject", {"x64_debug", "x64_release"});
    // vssln
    // .new_target("Target1")
    // .target_cpp_standard("Target1", makexx::target_cpp_standards::cpp20)
    // .target_c_standard("Target1", makexx::target_c_standards::c11)
    // .target_type("Target1", makexx::target_types::lib)
    // .target_sources("Target1", {"game\\lib.cpp"}, "Lib")
    // .target_headers("Target1", {"game\\lib.hpp"}, "Lib")
    // .target_config_defines("Target1", "x64_debug", {"MXX_DEBUG"})
    // .target_config_defines("Target1", "x64_release", {"MXX_RELEASE"})
    // .target_config_out_directory("Target1", "x64_release", "../bin/Target1/x64_release/")
    // .target_config_out_directory("Target1", "x64_debug", "../bin/Target1/x64_debug/")
    // .target_config_optimization("Target1", "x64_release", makexx::target_optimizations::o3)
    //
    // .new_target("Target0")
    // .target_cpp_standard("Target0", makexx::target_cpp_standards::cpp20)
    // .target_c_standard("Target0", makexx::target_c_standards::c11)
    // .target_type("Target0", makexx::target_types::exe)
    // .target_dependencies("Target0", {"Target1"})
    // .target_headers("Target0", {"game\\main.hpp"}, "Game")
    // .target_headers("Target0", {"game\\game.hpp"}, "Game")
    // .target_sources("Target0", {"game\\main.cpp"}, "Game\\Main")
    // .target_sources("Target0", {"game\\game.cpp"}, "Game\\Main")
    // .target_config_external_links("Target0", "x64_debug", {"OpenGL32"})
    // .target_config_external_links("Target0", "x64_release", {"OpenGL32"})
    // .target_config_defines("Target0", "x64_debug", {"MXX_DEBUG"})
    // .target_config_defines("Target0", "x64_release", {"MXX_RELEASE"})
    // .target_config_out_directory("Target0", "x64_release", "../bin/Target0/x64_release/")
    // .target_config_out_directory("Target0", "x64_debug", "../bin/Target0/x64_debug/")
    // .target_config_optimization("Target0", "x64_release", makexx::target_optimizations::o3)
    // .target_icon("Target0", "test_icon.ico");
    //
    // vssln.save_targets_to_files();
    // vssln.save_project_to_file();

    // std::ofstream ofs("makexx.generated.hpp");
    // ofs << makexx::put_header_archive_to_buffer();
    // ofs.close();
    
    // std::ifstream ifs("makexx.generated.hpp");
    //
    // std::string   cache;
    // for (std::string line; std::getline(ifs, line); ) {
    //     if (std::memcmp(line.c_str(), "#include", 8) != 0) {
    //         cache += line;
    //         cache.push_back('\n');
    //     }
    // }
    //
    // cpod::archive arch(cache);
    // std::unordered_map<std::string_view, std::string> map;
    // makexx::get_header_archive_from_buffer(arch, map);
    //
    // for (auto& [k, v] : map) {
    //     std::cout << k << " " << v << std::endl;
    // }
    //
    // return 0;
}
