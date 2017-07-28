#!/bin/bash
#
rm -rf CMakeCache.txt CMakeFiles/
cmake -DCMAKE_TOOLCHAIN_FILE=raspberrypi.cmake -DCMAKE_BUILD_TYPE=Release .
make clean all VERBOSE=1


