#!/bin/bash

find ./src/lib/cloudfuse/ -iname '*.h' -o -iname '*.cpp' | xargs 'clang-format' -i
find ./src/plugin/ -iname '*.h' -o -iname '*.cpp' | xargs 'clang-format' -i
find ./src/unit_tests/ -iname '*.h' -o -iname '*.cpp' | xargs 'clang-format' -i
find ./unit_tests/ -iname '*.h' -o -iname '*.cpp' | xargs 'clang-format' -i
