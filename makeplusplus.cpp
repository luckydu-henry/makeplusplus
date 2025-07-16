#include <algorithm>
#include <random>
#include <format>
#include <ranges>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "tinyxml2/tinyxml2.h"
#include "makeplusplus.hpp"

namespace makexx {
    
    namespace details {
        static std::string visual_studio_genguid() {
            std::random_device rd{};
            std::default_random_engine eng{ rd() };
            std::uniform_int_distribution<std::size_t> distrib{};
    
            return std::format("{{{:08X}-{:04X}-{:04X}-{:08X}{:04X}}}",
                distrib(eng) % 0xFFFFFFFF, distrib(eng) % 0xFFFF, distrib(eng) % 0xFFFF, distrib(eng) % 0xFFFF,
                distrib(eng) % 0xFFFFFFFF, distrib(eng) % 0xFFFF);
        }
        
        static auto  visual_studio_extract_config(std::string_view config) {
            std::size_t      split = config.find('_');
            std::string      mode(config.substr(split + 1));
            std::string      plat(config.substr(0, split));
            std::string      tag  = std::format("{:s}|{:s}", mode, plat);
            std::string      comb = std::format("'$(Configuration)|$(Platform)'=='{:s}'",tag);
            return std::make_tuple(mode, plat, tag, comb);
        }

        static void  visual_studio_gen_resource(std::filesystem::path iconrc, std::filesystem::path reschh, std::string_view iconname) {
            std::ofstream rc(iconrc), header(reschh);

            rc << std::format(R"(
// Microsoft Visual C++ generated resource script.
//
#include "{0:s}"

#define APSTUDIO_READONLY_SYMBOLS
/////////////////////////////////////////////////////////////////////////////
//
// Generated from the TEXTINCLUDE 2 resource.
//
#include "winres.h"

/////////////////////////////////////////////////////////////////////////////
#undef APSTUDIO_READONLY_SYMBOLS

#ifdef APSTUDIO_INVOKED
/////////////////////////////////////////////////////////////////////////////
//
// TEXTINCLUDE
//

1 TEXTINCLUDE 
BEGIN
    "{0:s}\0"
END

2 TEXTINCLUDE 
BEGIN
    "#include ""winres.h""\r\n"
    "\0"
END

3 TEXTINCLUDE 
BEGIN
    "\r\n"
    "\0"
END

#endif    // APSTUDIO_INVOKED


/////////////////////////////////////////////////////////////////////////////
//
// Icon
//

// Icon with lowest ID value placed first to ensure application icon
// remains consistent on all systems.
IDI_ICON1               ICON                    "{1:s}"

#ifndef APSTUDIO_INVOKED
/////////////////////////////////////////////////////////////////////////////
//
// Generated from the TEXTINCLUDE 3 resource.
//


/////////////////////////////////////////////////////////////////////////////
#endif    // not APSTUDIO_INVOKED)", reschh.generic_string(), iconname);

            header << R"(
//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
//
#define IDI_ICON1                       101

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        102
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1001
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif
)";
            rc.close();
            header.close();
        }

        static tinyxml2::XMLElement* xml_find_nth_child(std::string_view name, tinyxml2::XMLElement* root, std::size_t n = 0) {
            tinyxml2::XMLElement* element = root->FirstChildElement(name.data());
            for (std::size_t i = 0; i != n; ++i) {
                element = element->NextSiblingElement(name.data());
            }
            return element;
        }

        static void                  xml_save_map_to_file(std::unordered_map<std::string_view, tinyxml2::XMLDocument>& map, std::string_view ext, std::string_view rootdir = "") {
            for (auto& [target_name, doc] : map) {
                std::filesystem::path path = (std::filesystem::path(rootdir) / (std::string(target_name) + std::string(ext))).lexically_normal();
                if (doc.SaveFile(path.generic_string().c_str()) != tinyxml2::XML_SUCCESS) {
                    std::cerr << "Error saving " << ext << " file " << target_name << std::endl;
                    std::exit(EXIT_FAILURE);
                }
            }
        }
    }

    /////////////////////////////////////////////////////////////
    //     Header and Sources only have little differences.    //
    /////////////////////////////////////////////////////////////
    
    void visual_studio_project::target_attach_files_(std::string_view target_name, const std::vector<std::string>& files,
        std::string_view filter, AttachmentType type) {
        static const char* itemStrings[] = { "", "ClInclude", "ClCompile", "Image", "ResourceCompile" };
        
        // Add filter to list.
        tinyxml2::XMLDocument& docfilt = vcxproj_filters_map_[target_name];
        tinyxml2::XMLElement*  itemGroupFileFilter = details::xml_find_nth_child("ItemGroup", docfilt.RootElement(), type);
        
        if (!filter.empty() && !vcxproj_filter_name_map_.contains(filter)) {
            vcxproj_filter_name_map_[filter] = filter;
            details::xml_find_nth_child("ItemGroup", docfilt.RootElement())
            ->InsertNewChildElement("Filter")->SetAttribute("Include", filter.data())
            ->InsertNewChildElement("UniqueIdentifier")->SetText(details::visual_studio_genguid().c_str());
        }
        
        tinyxml2::XMLDocument& docproj = vcxproj_map_[target_name];
        tinyxml2::XMLElement*  itemGroupFiles = details::xml_find_nth_child("ItemGroup", docproj.RootElement(), type);

        for (auto& i : files) {
            itemGroupFiles->InsertNewChildElement(itemStrings[type])->SetAttribute("Include", i.c_str());
            
            tinyxml2::XMLElement* fclItem = itemGroupFileFilter->InsertNewChildElement(itemStrings[type]);
            fclItem->SetAttribute("Include", i.c_str());
            if (!filter.empty()) {
                fclItem->InsertNewChildElement("Filter")->SetText(filter.data());
            }
        }
    }
    

    visual_studio_project::visual_studio_project(std::string_view sln_name, const std::vector<std::string_view>& configs)
    : solution_name_(sln_name), solution_configs_(configs) {
        
    }

    visual_studio_project& visual_studio_project::new_target(std::string_view target_name) {

        ////////////////////////////////////////////
        //                Filters                ///
        ////////////////////////////////////////////
        tinyxml2::XMLDocument& docfilt = vcxproj_filters_map_[target_name];
        docfilt.InsertEndChild(docfilt.NewDeclaration("xml version=\"1.0\" encoding=\"utf-8\""));
        docfilt.InsertEndChild(docfilt.NewComment("Project generated by makeplusplus"));
        
        // Insert filter root project.
        tinyxml2::XMLElement* filter = docfilt.NewElement("Project");
        filter->SetAttribute("ToolsVersion", "4.0")->SetAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");
        docfilt.InsertEndChild(filter);

        filter->InsertNewComment("Global   filter defines")->ParentElement()->InsertNewChildElement("ItemGroup");
        filter->InsertNewComment("Header   filter defines")->ParentElement()->InsertNewChildElement("ItemGroup");
        filter->InsertNewComment("Source   filter defines")->ParentElement()->InsertNewChildElement("ItemGroup");
        filter->InsertNewComment("Icon     filter defines")->ParentElement()->InsertNewChildElement("ItemGroup");
        filter->InsertNewComment("Resource filter defines")->ParentElement()->InsertNewChildElement("ItemGroup");
        
        ////////////////////////////////////////////
        //                Project                ///
        ////////////////////////////////////////////
        vcxproj_guid_map_[target_name] = details::visual_studio_genguid();
        tinyxml2::XMLDocument& docproj = vcxproj_map_[target_name];

        docproj.InsertEndChild(docproj.NewDeclaration("xml version=\"1.0\" encoding=\"utf-8\""));
        docproj.InsertEndChild(docproj.NewComment("Project generated by makeplusplus"));

        // Insert project root declarations.
        tinyxml2::XMLElement* project = docproj.NewElement("Project");
        project->SetAttribute("DefaultTargets", "Build")->SetAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");
        docproj.InsertEndChild(project);

        // Insert ItemGroup project configurations
        tinyxml2::XMLElement* itemGroupConfig = project->InsertNewChildElement("ItemGroup")->SetAttribute("Label", "ProjectConfigurations");
        for (std::string_view config : solution_configs_) {
            auto[mode, plat, tag, comb] = details::visual_studio_extract_config(config);
            itemGroupConfig->InsertNewChildElement("ProjectConfiguration")->SetAttribute("Include", tag.c_str())
            ->InsertNewChildElement("Configuration")->SetText(mode.c_str()) ->ParentElement()
            ->InsertNewChildElement("Platform")     ->SetText(plat.c_str()) ->ParentElement();
        }

        // Insert global configurations
        project->InsertNewChildElement("PropertyGroup")->SetAttribute("Label", "Globals")
        ->InsertNewChildElement("ProjectGuid")                  ->SetText(vcxproj_guid_map_[target_name].c_str()) ->ParentElement()
        ->InsertNewChildElement("RootNamespace")                ->SetText(target_name.data())                     ->ParentElement()
        ->InsertNewChildElement("ProjectName")                  ->SetText(target_name.data())                     ->ParentElement()
        ->InsertNewChildElement("WindowsTargetPlatformVersion") ->SetText("10.0");
        
        // Import compile properties
        project->InsertNewChildElement("Import")->SetAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.Default.props");

        // Insert configuration.
        for (std::string_view config : solution_configs_) {
            auto[mode, plat, tag, comb] = details::visual_studio_extract_config(config);

            std::string mode_upper_norm;
            std::ranges::transform(mode, std::back_inserter(mode_upper_norm), [](auto i) { return std::toupper(i); });
            
            project->InsertNewChildElement("PropertyGroup")
            ->SetAttribute("Condition", comb.c_str())->SetAttribute("Label", "Configuration")
            ->InsertNewChildElement("ConfigurationType") ->SetText("Application") ->ParentElement()
            ->InsertNewChildElement("PlatformToolset")   ->SetText("v143")        ->ParentElement()
            ->InsertNewChildElement("CharacterSet")      ->SetText("Unicode")     ->ParentElement()
            ->InsertNewChildElement("UseDebugLibraries") ->SetText(mode_upper_norm == "DEBUG");
        }
        project->InsertNewChildElement("Import")->SetAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.props");

        // Import extension property list.
        project->InsertNewChildElement("ImportGroup")->SetAttribute("Label", "ExtensionSettings");
        project->InsertNewChildElement("ImportGroup")->SetAttribute("Label", "Shared");

        // Use for loop to set all config releated properties.
        for (std::string_view config : solution_configs_) {
            auto[mode, plat, tag, comb] = details::visual_studio_extract_config(config);
            project->InsertNewChildElement("ImportGroup")
            ->SetAttribute("Label", "PropertySheets")->SetAttribute("Condition", comb.c_str())
            ->InsertNewChildElement("Import")
            ->SetAttribute("Project", "$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props")
            ->SetAttribute("Condition", "exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')")
            ->SetAttribute("Label", "LocalAppDataPlatform");
        }

        // Insert item definitions.
        project->InsertNewChildElement("PropertyGroup")->SetAttribute("Label", "UserMacros");

        for (std::string_view config : solution_configs_) {
            auto[mode, plat, tag, comb] = details::visual_studio_extract_config(config);
            project->InsertNewChildElement("ItemDefinitionGroup")->SetAttribute("Condition", comb.c_str())
            ->InsertNewChildElement("ClCompile")->ParentElement()
            ->InsertNewChildElement("Link");
        }

        // include item group sequence 'project configuration''include' 'compile' 'icon' 'dependencies' 
        project->InsertNewComment("Include items")   ->ParentElement()->InsertNewChildElement("ItemGroup");
        project->InsertNewComment("Source items")    ->ParentElement()->InsertNewChildElement("ItemGroup");
        project->InsertNewChildElement("Import")     ->SetAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.targets");
        project->InsertNewChildElement("ImportGroup")->SetAttribute("Label", "ExtensionTargets");
        project->InsertNewComment("Icon Item")       ->ParentElement()->InsertNewChildElement("ItemGroup");
        project->InsertNewComment("Resource Item")   ->ParentElement()->InsertNewChildElement("ItemGroup");
        project->InsertNewComment("Dependency items")->ParentElement()->InsertNewChildElement("ItemGroup");
        
        return *this;
    }

    visual_studio_project& visual_studio_project::target_headers(std::string_view target_name,
        const std::vector<std::string>& headers, std::string_view filter) {
        target_attach_files_(target_name, headers, filter, Attach_Headers);
        return *this;        
    }

    visual_studio_project& visual_studio_project::target_sources(std::string_view target_name,
        const std::vector<std::string>& sources, std::string_view filter) {
        target_attach_files_(target_name, sources, filter, Attach_Sources);
        return *this;
    }

    visual_studio_project& visual_studio_project::target_icon(std::string_view target_name, std::string_view resource) {
        std::filesystem::path iconrc = std::filesystem::path(target_name).replace_extension("rc");
        std::filesystem::path rescrc = std::filesystem::path(target_name).replace_extension(".resource.h");
        details::visual_studio_gen_resource(iconrc, rescrc, resource);
        target_attach_files_(target_name, {std::string(resource)}, "", Attach_Icon);
        target_attach_files_(target_name, {iconrc.generic_string()}, "", Attach_Resource);
        return *this;
    }

    void visual_studio_project::save_targets_to_files(std::string_view root) {
        details::xml_save_map_to_file(vcxproj_map_, ".vcxproj", root);
        details::xml_save_map_to_file(vcxproj_filters_map_, ".vcxproj.filters", root);
    }
}
