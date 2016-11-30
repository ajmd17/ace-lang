cmake_minimum_required(VERSION 3.5)

file (GLOB_RECURSE ace_SOURCES "*.cpp")
add_executable(ace ${ace_SOURCES})
target_link_libraries(ace ace-c ace-vm)