cmake_minimum_required(VERSION 3.5)

file (GLOB_RECURSE ace_SOURCES "*.cpp")

foreach(f IN LISTS ace_SOURCES)
  get_filename_component(b ${f} NAME)
  set_source_files_properties(${f} PROPERTIES
    COMPILE_DEFINITIONS "__FILENAME__=${b}")
endforeach()

add_executable(ace ${ace_SOURCES})
target_link_libraries(ace ace-c ace-vm ace-runtime)