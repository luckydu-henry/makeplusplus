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

int main() {

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

    std::ofstream header("makexx.generated.hpp");
    header << makexx::put_header_archive_to_buffer({
        {"VULKAN_SDK", ""}
    });
    header.close();
    
    return 0;
}
