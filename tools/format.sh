#!/bin/bash

find ./samples/cloudfuse_plugin/ -iname '*.h' -o -iname '*.cpp' | xargs clang-format -i
find ./samples/unit_tests/ -iname '*.h' -o -iname '*.cpp' | xargs clang-format -i