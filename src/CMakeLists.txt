## Copyright © 2024 Seagate Technology LLC and/or its Affiliates
## Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

cmake_minimum_required(VERSION 3.16...3.29)

if(WIN32)
    set(VCPKG_TARGET_TRIPLET x64-windows-static)
endif()

project(
    cloudfuse_plugin
    VERSION 0.2.0
    DESCRIPTION "VMS Plugin for Lyve Cloud"
    HOMEPAGE_URL "https://github.com/Seagate/nx-lyve-cloud-plugin"
    LANGUAGES CXX
)

set(metadataSdkDir "" CACHE PATH "Path to unpacked VMS Metadata SDK zip.")
if(metadataSdkDir STREQUAL "")
    set(metadataSdkDir ${CMAKE_CURRENT_LIST_DIR}/..) #< Assume building samples inside the SDK.
    if(NOT EXISTS ${metadataSdkDir}/src/lib/nx/sdk OR NOT EXISTS ${metadataSdkDir}/dependencies/nx_kit/src/nx/kit)
        message(FATAL_ERROR "Define metadataSdkDir cache variable to point to the unzipped SDK.")
    endif()
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(UNIX)
    # In Linux, for the plugin .so library, set `rpath` to "$ORIGIN" and do not set `runpath`, thus
    # enabling the lookup of the dependencies in the plugin dir first.
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--disable-new-dtags")
endif()
set(CMAKE_SKIP_BUILD_RPATH ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

# Use all CPU cores by MSVC.
if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /MP")
    # Do not create separate .pdb files for object files (workaround for mspdbsrv.exe bug, needed
    # for using "/MP" flag).
    add_compile_options("/Z7")
endif()

if(WIN32)
    set(API_IMPORT_MACRO "__declspec(dllimport)")
    set(API_EXPORT_MACRO "__declspec(dllexport)")
else()
    set(API_IMPORT_MACRO "")
    set(API_EXPORT_MACRO "__attribute__((visibility(\"default\")))")
endif()

#--------------------------------------------------------------------------------------------------
# Define nx_kit lib, static.

set(nxKitLibraryType "STATIC" CACHE STRING "" FORCE)
set(nxKitWithTests "NO" CACHE STRING "" FORCE)

add_subdirectory(${metadataSdkDir}/dependencies/nx_kit ${CMAKE_CURRENT_BINARY_DIR}/dependencies/nx_kit)

#--------------------------------------------------------------------------------------------------
# Define nx_sdk lib, static, depends on nx_kit.

set(SDK_SRC_DIR ${metadataSdkDir}/src/lib)
file(GLOB_RECURSE SDK_SRC CONFIGURE_DEPENDS ${SDK_SRC_DIR}/*)

add_library(nx_sdk STATIC ${SDK_SRC})
target_include_directories(nx_sdk PUBLIC ${SDK_SRC_DIR})
target_link_libraries(nx_sdk PRIVATE nx_kit)

target_compile_definitions(nx_sdk PRIVATE NX_PLUGIN_API=${API_EXPORT_MACRO}) #< for nxLibContext()
target_compile_features(nx_sdk PRIVATE cxx_std_17)

#--------------------------------------------------------------------------------------------------
# Define cloudfuse_plugin lib, dynamic, depends on nx_kit and nx_sdk.
find_package(OpenSSL REQUIRED)
set(CLOUDFUSE_PLUGIN_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/plugin)
file(GLOB_RECURSE CLOUDFUSE_PLUGIN_SRC CONFIGURE_DEPENDS ${CLOUDFUSE_PLUGIN_SRC_DIR}/*)

add_library(cloudfuse_plugin SHARED ${CLOUDFUSE_PLUGIN_SRC})
target_include_directories(cloudfuse_plugin PRIVATE ${CLOUDFUSE_PLUGIN_SRC_DIR})
target_link_libraries(cloudfuse_plugin PRIVATE nx_kit nx_sdk OpenSSL::SSL)

target_compile_definitions(cloudfuse_plugin
    PRIVATE NX_PLUGIN_API=${API_EXPORT_MACRO}
)

target_compile_features(cloudfuse_plugin PRIVATE cxx_std_17)
set_target_properties(cloudfuse_plugin PROPERTIES 
    CXX_EXTENSIONS OFF
    NTERPROCEDURAL_OPTIMIZATION TRUE
)

if(NOT MSVC)
    target_compile_options(cloudfuse_plugin PRIVATE
        -Wall -Wformat -Wformat=2 -Wconversion -Wimplicit-fallthrough
        -Werror=format-security
    )
else()
    add_compile_definitions(_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
    target_compile_options(cloudfuse_plugin PRIVATE
        -W4 -GS -sdl
    )
endif()

if(NOT WIN32)
    target_link_libraries(cloudfuse_plugin PRIVATE pthread)
endif()