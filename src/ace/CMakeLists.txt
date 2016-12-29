cmake_minimum_required(VERSION 3.5)

file (GLOB_RECURSE ace_SOURCES "*.cpp")
file (GLOB_RECURSE ace_HEADERS "../../include/ace/*.hpp")

foreach(f IN LISTS ace_SOURCES)
  get_filename_component(b ${f} NAME)
  set_source_files_properties(${f} PROPERTIES
    COMPILE_DEFINITIONS "__FILENAME__=${b}")
endforeach()
foreach(f IN LISTS ace_HEADERS)
  get_filename_component(b ${f} NAME)
  set_source_files_properties(${f} PROPERTIES
    COMPILE_DEFINITIONS "__FILENAME__=${b}")
endforeach()

find_package(Threads)

add_executable(ace ${ace_SOURCES} ${ace_HEADERS})
target_link_libraries(ace ace-c ace-vm ace-runtime ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
