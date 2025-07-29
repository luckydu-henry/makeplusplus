#include "makeplusplus/makexx.generated.hpp"

PROJECT_NAME            = "SampleProject";
PROJECT_TARGETS         = {"Target0", "Target1"};
PROJECT_CONFIGURATIONS  = { 
    "x64_debug",
    "x64_release" 
};

// Next define each target.
// Use property to declare what config and platform this project can be built to.
namespace Target0 {
    // Use different config according to different config.
    // Name is the same as above.
    namespace x64_debug {

    }

    namespace x64_release {

    }

    namespace x64_minsizerel {

    }

}