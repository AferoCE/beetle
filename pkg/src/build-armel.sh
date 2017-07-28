#!/bin/bash
#
rm -rf CMakeCache.txt CMakeFiles/
cmake -DCMAKE_TOOLCHAIN_FILE=armel.cmake -DCMAKE_BUILD_TYPE=Release .
make clean all VERBOSE=1


