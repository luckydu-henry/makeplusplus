#include "tinyxml2/tinyxml2.h"
#include <iostream>
#include <string>
#include <random>
#include <format>

#include "tinyxml2/tinyxml2.h"
#include "makeplusplus.hpp"

using namespace tinyxml2;

int main() {

    makexx::visual_studio_project vssln("SampleProject", {"x64_debug", "x64_release"});
    vssln
    .new_target("Target1")
    .target_cpp_standard("Target1", makexx::target_cpp_standards::cpp20)
    .target_c_standard("Target1", makexx::target_c_standards::c11)
    .target_type("Target1", makexx::target_types::lib)
    .target_sources("Target1", {"game\\lib.cpp"}, "Lib")
    .target_headers("Target1", {"game\\lib.hpp"}, "Lib")
    .target_config_defines("Target1", "x64_debug", {"MXX_DEBUG"})
    .target_config_defines("Target1", "x64_release", {"MXX_RELEASE"})
    .target_config_optimization("Target1", "x64_release", makexx::target_optimizations::o3)

    .new_target("Target0")
    .target_cpp_standard("Target0", makexx::target_cpp_standards::cpp20)
    .target_c_standard("Target0", makexx::target_c_standards::c11)
    .target_type("Target0", makexx::target_types::exe)
    .target_dependencies("Target0", {"Target1"})
    .target_headers("Target0", {"game\\main.hpp"}, "Game")
    .target_headers("Target0", {"game\\game.hpp"}, "Game")
    .target_sources("Target0", {"game\\main.cpp"})
    .target_sources("Target0", {"game\\game.cpp"})
    .target_external_links("Target0", "x64_debug", {"OpenGL32"})
    .target_external_links("Target0", "x64_release", {"OpenGL32"})
    .target_config_defines("Target0", "x64_debug", {"MXX_DEBUG"})
    .target_config_defines("Target0", "x64_release", {"MXX_RELEASE"})
    .target_config_optimization("Target0", "x64_release", makexx::target_optimizations::o3)
    .target_icon("Target0", "test_icon.ico");

    vssln.save_targets_to_files();
    vssln.save_project_to_file();

    return 0;
}
