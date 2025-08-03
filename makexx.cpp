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

int main(int argc, char** argv) {
    try {
        makexx::make_application app(argc, argv);
        return app();
    } catch (std::exception& e) {
        std::cerr << std::format("Exception caused: {:s}", e.what()) << std::endl;
    }
}
