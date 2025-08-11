#pragma once

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#include <unordered_map>
#include <vector>
#include <string>
#include <format>
#include <ostream>
#include "xmloxx.hpp"

namespace msvc_xml {
    class document;
    class element;
}

namespace cpod {
    class archive;
}

namespace makexx {

    // C++'s std::print will cause program size inflate
    // This alternative is better.
    template <typename ... Args>
    inline void tiny_print(std::ostream& f, std::string_view fmt, Args&& ... args) {
        auto buf = std::vformat(fmt, std::make_format_args(args...));
        f.rdbuf()->sputn(buf.data(), buf.size());
    }

    enum class target_types             : std::uint32_t { exe     = 1, lib, dll };
    enum class target_cpp_standards     : std::uint32_t { latest  = 1, cpp11, cpp14, cpp17, cpp20, cpp23, cpp26, };
    enum class target_c_standards       : std::uint32_t { latest  = 1, c11, c17, c23 };
    enum class target_optimizations     : std::uint32_t { o0      = 1, o1, o2, o3 };
    enum class target_msvc_subsystems   : std::uint32_t { console = 1, window };
    
    class visual_studio_project {
        std::string                                             solution_name_;
        std::vector<std::string>                                solution_configs_;
        // For visual studio project locate.
        std::unordered_map<std::string_view, std::string>       vcxproj_guid_map_;
        std::unordered_map<std::string_view, xmloxx::tree>      vcxproj_map_;
        std::unordered_map<std::string_view, xmloxx::tree>      vcxproj_filters_map_;
        std::unordered_map<std::string_view, std::string_view>  vcxproj_filter_name_map_;

        enum AttachmentType {
            Attach_Headers        = 1,
            Attach_Sources        = 2,
            Attach_Icon           = 3,
            Attach_Resource       = 4,
            Attach_Dependency     = 5
        };
        
    public:
        visual_studio_project(std::string_view sln_name, const std::vector<std::string>& configs);
        
        visual_studio_project(const visual_studio_project&)            noexcept = default;
        visual_studio_project(visual_studio_project&&)                 noexcept = default;
        visual_studio_project& operator=(const visual_studio_project&) noexcept = default;
        visual_studio_project& operator=(visual_studio_project&&)      noexcept = default;

        /////////////////////////////////////////////////////////////////////
        //                   Common target properties.                     //
        /////////////////////////////////////////////////////////////////////
        visual_studio_project& new_target           (std::string_view target_name);
        visual_studio_project& target_headers       (std::string_view target_name, const std::vector<std::string>& headers, const std::string& filter = "");
        visual_studio_project& target_sources       (std::string_view target_name, const std::vector<std::string>& sources, const std::string& filter = "");
        visual_studio_project& target_msvc_icon     (std::string_view target_name, std::string_view         resource);
        visual_studio_project& target_dependencies  (std::string_view target_name, const std::vector<std::string>& dependencies);
        
        visual_studio_project& target_type           (std::string_view target_name, target_types          type);
        visual_studio_project& target_std_cpp        (std::string_view target_name, target_cpp_standards  version);
        visual_studio_project& target_std_c          (std::string_view target_name, target_c_standards    version);
        
        visual_studio_project& target_msvc_subsystem      (std::string_view target_name, target_msvc_subsystems sys);
        
        visual_studio_project& target_optimization           (std::string_view target_name, target_optimizations   op, std::string_view config);
        visual_studio_project& target_defines                (std::string_view target_name, const std::vector<std::string>& defines, std::string_view config); // Use ; as separator
        visual_studio_project& target_external_links         (std::string_view target_name, const std::vector<std::string>& links, std::string_view config);
        visual_studio_project& target_binary_directory       (std::string_view target_name, std::string_view dir, std::string_view config);
        visual_studio_project& target_intermediate_directory (std::string_view target_name, std::string_view dir, std::string_view config);
        //
        visual_studio_project& target_external_link_directories    (std::string_view target_name, const std::vector<std::string>& dirs);
        visual_studio_project& target_external_include_directories (std::string_view target_name, const std::vector<std::string>& dirs);

        void                   save_project_to_file(std::string_view root = "");
        void                   save_targets_to_files(std::string_view root = "");
    };
        
    class makefile_project {
        
    };

    class make_application {

        static constexpr std::string_view s_hello_message =
R"(---------------------------------------------------------------------------------------------------------------------
Hello, welcome to use makeplusplus!
This software allows you to create visual studio solution project and makefile project with 'C++' code!
The software itself is also written in C++ and that results in its high performance.
This software is especially for those developers who want to have a lightweight portable building system
with a good speed, if you are building an application like a video game or an editor, you are going to love this,
however if your goal is to develop a third-party library and wants to include dependencies or some high-end features
easily, you might still want to use CMake.
You can use -h or --help command for more usages.
To create a makeplusplus project, just use -gp and you are ready to go.
---------------------------------------------------------------------------------------------------------------------)";

        static constexpr std::string_view s_help_message =
R"(---------------------------------------------------------------------------------------------------------------------
-h/--help                : This command.
-gh                      : Generate only platform dependent header with makeplusplus project structure.
-gp <project-name>       : Generate complete project with makeplusplus project structure.
-gv <description-path>   : Generate visual studio solution and projects under '<project>' folder.
---------------------------------------------------------------------------------------------------------------------)";
        
        int         argc_;
        char**      argv_;

        std::unordered_map<std::string_view, std::string> definition_map_;

        // Basic informations.
        std::string                  mxx_project_name;
        std::vector<std::string>     mxx_project_targets;
        std::vector<std::string>     mxx_project_configurations;

        std::unordered_map<std::string, std::string> mxx_project_source_fields_;

        void generate_header_();
        void generate_project_();
        void read_current_definition_map_();
        void read_source_and_split_targets_();
        void read_target_and_generate_vs_project_(const std::string& target, const std::string& source, visual_studio_project& vssln);
        void generate_actual_visual_studio_project_();
        
    public:
        make_application(int argc, char** argv);
        ~make_application() = default;

        int operator()();
    };
    
}