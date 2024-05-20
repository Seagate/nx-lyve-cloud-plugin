cmake_minimum_required(VERSION 3.12...3.29)

if(WIN32)
    set(CMAKE_CXX_COMPILER "C:/msys64/ucrt64/bin/g++.exe")
    set(CMAKE_C_COMPILER "C:/msys64/ucrt64/bin/gcc.exe")
endif()

project(
    cloudfuse_storage_plugin
    VERSION 0.1.0)

add_library(ChildProcess STATIC child_process.h child_process_linux.cpp child_process_windows.cpp)                      
add_executable(main main.cpp)

target_compile_features(main PUBLIC cxx_std_17)
set_target_properties(main PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(main PRIVATE ChildProcess)
