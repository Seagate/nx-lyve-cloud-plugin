cmake_minimum_required(VERSION 3.12...3.29)

project(
    cloudfuse_storage_plugin
    VERSION 0.1.0)

add_library(ChildProcess STATIC child_process.h child_process_linux.cpp child_process_windows.cpp)                      
add_executable(main main.cpp)

target_link_libraries(main PRIVATE ChildProcess)
