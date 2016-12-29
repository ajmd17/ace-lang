cmake_minimum_required(VERSION 3.5)

file (GLOB_RECURSE ace-runtime_SOURCES "*.cpp")
file (GLOB_RECURSE ace-runtime_HEADERS "../../include/ace-runtime/*.hpp")

foreach(f IN LISTS ace-runtime_SOURCES)
    get_filename_component(b ${f} NAME)
    set_source_files_properties(${f} PROPERTIES
            COMPILE_DEFINITIONS "__FILENAME__=${b}")
endforeach()
foreach(f IN LISTS ace-runtime_HEADERS)
    get_filename_component(b ${f} NAME)
    set_source_files_properties(${f} PROPERTIES
            COMPILE_DEFINITIONS "__FILENAME__=${b}")
endforeach()

add_library(ace-runtime SHARED ${ace-runtime_SOURCES} ${ace-runtime_HEADERS})
target_link_libraries(ace-runtime ace-vm)
