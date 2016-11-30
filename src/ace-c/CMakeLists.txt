cmake_minimum_required(VERSION 3.5)

file (GLOB_RECURSE ace-c_SOURCES "*.cpp")
add_library(ace-c STATIC ${ace-c_SOURCES})