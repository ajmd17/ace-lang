cmake_minimum_required(VERSION 3.5)

file (GLOB_RECURSE ace-jit_SOURCES "*.cpp")

foreach(f IN LISTS ace-jit_SOURCES)
  get_filename_component(b ${f} NAME)
  set_source_files_properties(${f} PROPERTIES
    COMPILE_DEFINITIONS "__FILENAME__=${b}")
endforeach()

add_library(ace-jit STATIC ${ace-jit_SOURCES})
