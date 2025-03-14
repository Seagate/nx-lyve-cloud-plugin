#!/bin/bash

## Copyright © 2024 Seagate Technology LLC and/or its Affiliates
## Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

set -e #< Exit on error.
set -u #< Prohibit undefined variables.

BASE_DIR=$(readlink -f "$(dirname "$0")") #< Absolute path to the current dir.

set -x #< Log each command.

"$BASE_DIR"/build_plugin.sh \
    --no-tests \
    --edge-build \
    -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++-9 \
    -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc-9 \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT"/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=arm64-linux \
    -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$BASE_DIR/toolchain_arm64.cmake" \
    "$@"
