#pragma once

#include <unordered_map>
#include <vector>
#include <string>

namespace tinyxml2 {
    class XMLDocument;
    class XMLElement;
}

namespace makexx {
    
    class visual_studio_project {
        std::string                                                   solution_name_;
        std::vector<std::string_view>                                 solution_configs_;
        // For visual studio project locate.
        std::unordered_map<std::string_view, std::string>             vcxproj_guid_map_;
        std::unordered_map<std::string_view, tinyxml2::XMLDocument>   vcxproj_map_;
        std::unordered_map<std::string_view, tinyxml2::XMLDocument>   vcxproj_filters_map_;
        std::unordered_map<std::string_view, std::string_view>        vcxproj_filter_name_map_;

        enum AttachmentType {
            Attach_Headers        = 1,
            Attach_Sources        = 2,
            Attach_Icon           = 3,
            Attach_Resource       = 4,
            Attach_Dependency     = 5
        };
        
        void target_attach_files_(std::string_view target_name, const std::vector<std::string>& files, std::string_view filter, AttachmentType type);
    public:
        visual_studio_project(std::string_view sln_name, const std::vector<std::string_view>& configs);
        
        visual_studio_project(const visual_studio_project&)            noexcept = default;
        visual_studio_project(visual_studio_project&&)                 noexcept = default;
        visual_studio_project& operator=(const visual_studio_project&) noexcept = default;
        visual_studio_project& operator=(visual_studio_project&&)      noexcept = default;

        /////////////////////////////////////////////////////////////////////
        //                   Common target properties.                     //
        /////////////////////////////////////////////////////////////////////
        visual_studio_project& new_target           (std::string_view target_name);
        visual_studio_project& target_headers       (std::string_view target_name, const std::vector<std::string>& headers, std::string_view filter = "");
        visual_studio_project& target_sources       (std::string_view target_name, const std::vector<std::string>& sources, std::string_view filter = "");
        visual_studio_project& target_icon          (std::string_view target_name, std::string_view         resource);
        visual_studio_project& target_dependencies  (std::string_view target_name, std::vector<std::string> dependencies);
        
        visual_studio_project& target_type          (std::string_view target_name, std::uint32_t            type);
        visual_studio_project& target_cpp_version   (std::string_view target_name, std::uint32_t            version);
        visual_studio_project& target_c_version     (std::string_view target_name, std::uint32_t            version);
        
        visual_studio_project& target_config_optimization (std::string_view target_name, std::string_view config, std::uint32_t     level);
        visual_studio_project& target_config_defines      (std::string_view target_name, std::string_view config, std::string_view  defines); // Use ; as seperator
        visual_studio_project& target_config_intrinsics   (std::string_view target_name, std::string_view config, std::uint32_t     intrin);
        visual_studio_project& target_config_subsystem    (std::string_view target_name, std::string_view config, std::uint32_t     type);

        void                   save_project_to_file(std::string_view root = "");
        void                   save_targets_to_files(std::string_view root = "");

        std::vector<std::string_view>& configs() { return solution_configs_; };
    };
        
    class makefile_project {
        
    };
    
}