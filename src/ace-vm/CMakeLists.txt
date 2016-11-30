cmake_minimum_required(VERSION 3.5)

file (GLOB_RECURSE ace-vm_SOURCES "*.cpp")
add_library(ace-vm STATIC ${ace-vm_SOURCES})