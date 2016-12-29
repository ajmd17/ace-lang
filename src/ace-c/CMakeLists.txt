cmake_minimum_required(VERSION 3.5)

file (GLOB_RECURSE ace-c_SOURCES "*.cpp")
file (GLOB_RECURSE ace-c_HEADERS "../../include/ace-c/*.hpp")

foreach(f IN LISTS ace-c_SOURCES)
  get_filename_component(b ${f} NAME)
  set_source_files_properties(${f} PROPERTIES
    COMPILE_DEFINITIONS "__FILENAME__=${b}")
endforeach()
foreach(f IN LISTS ace-c_HEADERS)
  get_filename_component(b ${f} NAME)
  set_source_files_properties(${f} PROPERTIES
    COMPILE_DEFINITIONS "__FILENAME__=${b}")
endforeach()

add_library(ace-c STATIC ${ace-c_SOURCES} ${ace-c_HEADERS})