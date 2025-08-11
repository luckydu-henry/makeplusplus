#include "makeplusplus.hpp"
#include "xmloxx.hpp"

int main(int argc, char** argv) {
    makexx::make_application app(argc, argv);
    return app();
}