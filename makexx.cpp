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
    .target_type("Target1", makexx::target_type::lib)
    .target_sources("Target1", {"game\\lib.cpp"}, "Lib")
    .target_headers("Target1", {"game\\lib.hpp"}, "Lib")

    .new_target("Target0")
    .target_type("Target0", makexx::target_type::exe)
    .target_dependencies("Target0", {"Target1"})
    .target_headers("Target0", {"game\\main.hpp"}, "Game")
    .target_headers("Target0", {"game\\game.hpp"}, "Game")
    .target_sources("Target0", {"game\\main.cpp"})
    .target_sources("Target0", {"game\\game.cpp"})
    .target_icon("Target0", "test_icon.ico");

    vssln.save_targets_to_files();
    vssln.save_project_to_file();

    return 0;
}
