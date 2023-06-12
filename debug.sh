#!/bin/bash
./build.sh
gdb -x gdbinit ./build/bin/cats_cradle
