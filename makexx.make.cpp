// You can not have this line actually, this is only for IDE syntax-highlighting or auto-complete.
#include "makexx/makexx.generated.hpp"

// Must fill these three properties first.
PROJECT_NAME            = "makeplusplus";
PROJECT_TARGETS         = {"makexx"};

// Configurations are all form of "<architecture>_<build-mode>"
// You can choose "build-mode" whatever you like, not limited to "debug" or "release"
PROJECT_CONFIGURATIONS  = { 
    "x64_debug",
    "x64_release" 
};

// This line is necessary, it's actually a seperator.
// All namespaces below must have exact same names as elements in targets.
// There is a bug here, comments are not allowed between #pragma target_definitions and namespace.
// I will fix it.
#pragma target_definitions

namespace makexx {
    // Must fill global properties first.
    // We use MXX_PROJECT_ROOT as root path, check makexx.generated.hpp for more details.
    // If you place your make.cpp in any folder besides parent folder, you must change MXX_PROJECT_ROOT manually.
    TARGET_SOURCES = {
        "makexx.cpp", "makeplusplus.cpp"
    };
    TARGET_HEADERS = {
        "cpod.hpp", "makeplusplus.hpp", "xmloxx.hpp"
    };
    TARGET_TYPE                         = MXX_TARGET_TYPE_EXE;

    // These four attributes must be defined together.
    // This is a bug and I'm still working on to fix it.
    TARGET_STD_CPP                      = MXX_STD_CPP20;
    TARGET_STD_C                        = MXX_STD_C11;
    TARGET_EXTERNAL_INCLUDE_DIRECTORIES = {""};
    TARGET_EXTERNAL_LINK_DIRECTORIES    = {""};

    // After filled all global properties we can start filled config specific properties.
    namespace x64_debug {
        TARGET_OPTIMIZATION           = MXX_OPTIMIZATION_0;
        TARGET_BINARY_DIRECTORY       = "build/bin/x64_debug/";
        TARGET_INTERMEDIATE_DIRECTORY = "build/int/x64_debug/";
    }
    namespace x64_release {
        TARGET_OPTIMIZATION           = MXX_OPTIMIZATION_1;
        TARGET_BINARY_DIRECTORY       = "build/bin/x64_release/";
        TARGET_INTERMEDIATE_DIRECTORY = "build/int/x64_release/";
    }
}