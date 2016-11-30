cmake_minimum_required(VERSION 3.5)

project(ace-lang)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/include
)

add_subdirectory(src)


#project(ace)

  
#file (GLOB_RECURSE ace_SOURCES "src/ace/*.cpp")
#include_directories(
#  ${CMAKE_CURRENT_LIST_DIR}/include
#  )
#add_executable(aceExe ${ace_SOURCES})