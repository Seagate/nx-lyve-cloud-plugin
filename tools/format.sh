#!/bin/bash

find ./src/lib/cloudfuse/ -iname '*.h' -o -iname '*.cpp' | xargs 'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\Llvm\bin\clang-format.exe' -i
find ./src/plugin/ -iname '*.h' -o -iname '*.cpp' | xargs 'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\Llvm\bin\clang-format.exe' -i
find ./src/unit_tests/ -iname '*.h' -o -iname '*.cpp' | xargs 'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\Llvm\bin\clang-format.exe' -i
find ./unit_tests/ -iname '*.h' -o -iname '*.cpp' | xargs 'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\Llvm\bin\clang-format.exe' -i
