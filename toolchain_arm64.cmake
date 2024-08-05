## Copyright © 2024 Seagate Technology LLC and/or its Affiliates
## Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

# Clang supports cross compiling, but needs to specify the target to compile to
if(CMAKE_C_COMPILER MATCHES "clang")
    set(CMAKE_C_FLAGS "--target=aarch64-linux-gnu")
    set(CMAKE_CXX_FLAGS "--target=aarch64-linux-gnu")
endif()

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_CROSSCOMPILING TRUE)

set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

# This is required by FindThreads CMake module.
set(THREADS_PTHREAD_ARG "2" CACHE STRING "" FORCE)
mark_as_advanced(THREADS_PTHREAD_ARG)
