#pragma once

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#include <unordered_map>
#include <vector>
#include <string>

namespace tinyxml2 {
    class XMLDocument;
    class XMLElement;
}

namespace cpod {
    class archive;
}

namespace makexx {

    enum class target_types             : std::uint32_t { exe     = 1, lib, dll };
    enum class target_cpp_standards     : std::uint32_t { latest  = 1, cpp11, cpp14, cpp17, cpp20, cpp23, cpp26, };
    enum class target_c_standards       : std::uint32_t { latest  = 1, c11, c17, c23 };
    enum class target_optimizations     : std::uint32_t { o0      = 1, o1, o2, o3 };
    enum class target_msvc_subsystems   : std::uint32_t { console = 1, window };

    void         put_default_content(cpod::archive& arch);
    std::string  put_header_archive_to_buffer(const std::unordered_map<std::string_view, std::string>& defmap = {});
    void         get_header_archive_from_buffer(cpod::archive& arch, std::unordered_map<std::string_view, std::string>& defmap);
    
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
        
        void target_attach_files_             (std::string_view target_name, const std::vector<std::string>& files, std::string_view filter, AttachmentType type);
        void target_set_item_definition_group_(std::string_view target_name, std::string_view condition, std::string_view scope, std::string_view elem, std::string_view value);
        void target_append_property_group_    (std::string_view target_name, std::string_view condition, std::string_view scope, std::string_view value);
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
        visual_studio_project& target_dependencies  (std::string_view target_name, const std::vector<std::string>& dependencies);
        
        visual_studio_project& target_type                (std::string_view target_name, target_types          type);
        visual_studio_project& target_cpp_standard        (std::string_view target_name, target_cpp_standards  version);
        visual_studio_project& target_c_standard          (std::string_view target_name, target_c_standards    version);
        
        visual_studio_project& target_msvc_subsystem      (std::string_view target_name, target_msvc_subsystems sys);
        
        visual_studio_project& target_config_optimization        (std::string_view target_name, std::string_view config, target_optimizations   op);
        visual_studio_project& target_config_defines             (std::string_view target_name, std::string_view config, const std::vector<std::string>& defines); // Use ; as separator
        visual_studio_project& target_config_external_links      (std::string_view target_name, std::string_view config, const std::vector<std::string>& links);
        visual_studio_project& target_config_out_directory       (std::string_view target_name, std::string_view config, std::string_view dir);
        visual_studio_project& target_config_int_directory       (std::string_view target_name, std::string_view config, std::string_view dir);
        
        visual_studio_project& target_link_directories    (std::string_view target_name, const std::vector<std::string>& dirs);
        visual_studio_project& target_include_directories (std::string_view target_name, const std::vector<std::string>& dirs);

        void                   save_project_to_file(std::string_view root = "");
        void                   save_targets_to_files(std::string_view root = "");

        std::vector<std::string_view>& configs() { return solution_configs_; };
    };
        
    class makefile_project {
        
    };
    
}