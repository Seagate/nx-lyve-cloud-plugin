#!/bin/bash

## Copyright © 2024 Seagate Technology LLC and/or its Affiliates
## Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

set -e #< Exit on error.
set -u #< Prohibit undefined variables.

if [[ $# > 0 && ($1 == "/?" || $1 == "-h" || $1 == "--help") ]]
then
    echo "Usage: $(basename "$0") [--no-tests] [--debug] [<cmake-generation-args>...]"
    echo " --debug Compile using Debug configuration (without optimizations) instead of Release."
    exit
fi

# Make the build dir at the same level as the parent dir of this script, suffixed with "-build".
BASE_DIR=$(readlink -f "$(dirname "$0")") #< Absolute path to this script's dir.
BUILD_DIR="$BASE_DIR-build"

if [[ $# > 0 && $1 == "--no-tests" ]]
then
    shift
    NO_TESTS=1
else
    NO_TESTS=0
fi

if [[ $# > 0 && $1 == "--debug" ]]
then
    shift
    BUILD_TYPE=Debug
else
    BUILD_TYPE=Release
fi

case "$(uname -s)" in #< Check if running in Windows from Cygwin/MinGW.
    CYGWIN*|MINGW*)
        GEN_OPTIONS=( -G "MinGW Makefiles" -DCMAKE_C_COMPILER="C:/msys64/ucrt64/bin/gcc.exe" -DCMAKE_CXX_COMPILER="C:/msys64/ucrt64/bin/g++.exe" )
        BASE_DIR=$(cygpath -w "$BASE_DIR") #< Windows-native cmake requires Windows path.
        BUILD_OPTIONS=()
        if [[ $BUILD_TYPE == Release ]]
        then
            BUILD_OPTIONS+=( --config $BUILD_TYPE )
        fi
        ;;
    *) # Assume Linux; use Ninja if it is available on PATH.
        if which ninja >/dev/null
        then
            GEN_OPTIONS=( -GNinja ) #< Generate for Ninja and gcc; Ninja uses all CPU cores.
            BUILD_OPTIONS=()
        else
            GEN_OPTIONS=() #< Generate for GNU make and gcc.
            BUILD_OPTIONS=( -- -j ) #< Use all available CPU cores.
        fi
        if [[ $BUILD_TYPE == Release ]]
        then
            GEN_OPTIONS+=( -DCMAKE_BUILD_TYPE=$BUILD_TYPE )
        fi
        ;;
esac

(set -x #< Log each command.
    # rm -rf "$BUILD_DIR/"
)

# Build source plugin
SOURCE_DIR="$BASE_DIR/src"
PLUGIN_NAME="cloudfuse_plugin"
(set -x #< Log each command.
    mkdir -p "$BUILD_DIR/$PLUGIN_NAME"
    cd "$BUILD_DIR/$PLUGIN_NAME"

    cmake "$SOURCE_DIR" `# allow empty array #` ${GEN_OPTIONS[@]+"${GEN_OPTIONS[@]}"} "$@"
    cmake --build . `# allow empty array #` ${BUILD_OPTIONS[@]+"${BUILD_OPTIONS[@]}"}
)

ARTIFACT=$(find "$BUILD_DIR" -name "$PLUGIN_NAME.dll" -o -name "lib$PLUGIN_NAME.so")
if [ ! -f "$ARTIFACT" ]
then
    echo "ERROR: Failed to build $PLUGIN_NAME."
    exit 64
fi
echo ""
echo "Built: $ARTIFACT"
echo ""

# Build unit tests
SOURCE_DIR="$BASE_DIR/src/unit_tests"
UNIT_TESTS="unit_tests"
(set -x #< Log each command.
    mkdir -p "$BUILD_DIR/$UNIT_TESTS"
    cd "$BUILD_DIR/$UNIT_TESTS"

    cmake "$SOURCE_DIR" `# allow empty array #` ${GEN_OPTIONS[@]+"${GEN_OPTIONS[@]}"} "$@"
    cmake --build . `# allow empty array #` ${BUILD_OPTIONS[@]+"${BUILD_OPTIONS[@]}"}
)

ARTIFACT=$(find "$BUILD_DIR" -name "analytics_plugin_ut.exe" -o -name "analytics_plugin_ut")
if [ ! -f "$ARTIFACT" ]
then
    echo "ERROR: Failed to build $UNIT_TESTS."
    exit 64
fi
echo ""
echo "Built: $ARTIFACT"
echo ""

# Run unit tests if needed.
if [[ $NO_TESTS == 1 ]]
then
    echo "NOTE: Unit tests were not run."
else
    (set -x #< Log each command.
        cd "$BUILD_DIR/unit_tests"
        ctest --output-on-failure -C $BUILD_TYPE
    )
fi
echo ""

echo "Samples built successfully, see the binaries in $BUILD_DIR"
