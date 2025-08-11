#include "makeplusplus.hpp"
#include "xmloxx.hpp"
#include <iostream>

int main(int argc, char** argv) {
    return makexx::make_application(argc, argv)();
}