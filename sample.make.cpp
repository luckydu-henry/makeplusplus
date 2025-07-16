#include <vector>
#include <string>

#define PROJECT_NAME                 std::string              mxx_project_names
#define PROJECT_TARGET_NAMES         std::vector<std::string> mxx_project_target_names
#define PROJECT_CONFIGURATIONS       std::vector<std::string> mxx_project_configs

// Descript a project must define these before actually descript a target
PROJECT_NAME               = "SampleProject";
PROJECT_TARGET_NAMES       = {"Target0", "Target1"};
// On VisualStudio name before _ will be parsed as "Platform" and name after _ will be parsed as "Configuration"
// So you can not have _ at least in your platform scope.
PROJECT_CONFIGURATIONS     = {"x64_debug", "x64_release", "x64_minsizerel"};

// Next define each target.
// Use property to declare what config and platform this project can be built to.
namespace Target0 {
    // Can also be 
    #ifdef PROJECT_CURRENT_PLATFORM_WINDOWS
    #endif


    // Use different config according to different config.
    // Name is the same as above.
    namespace x64_debug {

    }

    namespace x64_release {

    }

    namespace x64_minsizerel {

    }

}

namespace Target1 {

}