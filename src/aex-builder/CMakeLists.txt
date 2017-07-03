cmake_minimum_required(VERSION 3.5)

file (GLOB_RECURSE aex-builder_SOURCES "*.cpp")
file (GLOB_RECURSE aex-builder_HEADERS "../../include/aex-builder/*.hpp")

foreach(f IN LISTS aex-builder_SOURCES)
  get_filename_component(b ${f} NAME)
  set_source_files_properties(${f} PROPERTIES
    COMPILE_DEFINITIONS "__FILENAME__=${b}")
endforeach()
foreach(f IN LISTS aex-builder_HEADERS)
  get_filename_component(b ${f} NAME)
  set_source_files_properties(${f} PROPERTIES
    COMPILE_DEFINITIONS "__FILENAME__=${b}")
endforeach()

add_library(aex-builder STATIC ${aex-builder_SOURCES} ${aex-builder_HEADERS})