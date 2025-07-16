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
    .new_target("Target0")
    .target_headers("Target0", {"game\\main.hpp"}, "Game")
    .target_headers("Target0", {"game\\game.hpp"}, "Game")
    .target_sources("Target0", {"game\\main.cpp"})
    .target_sources("Target0", {"game\\game.cpp"})
    .target_icon("Target0", "test_icon.ico");
    
    vssln.save_targets_to_files();

    return 0;
}
