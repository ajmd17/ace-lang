cmake_minimum_required(VERSION 3.5)

project(ace-lang)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXECUTABLE_OUTPUT_PATH "bin")

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/../include
)

if(MSVC)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

add_subdirectory(ace-c)
add_subdirectory(ace-vm)
add_subdirectory(ace-runtime)
add_subdirectory(ace)

