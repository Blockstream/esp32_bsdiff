#!/bin/bash
set -eo pipefail

# we clean up the dir directory just in case and sdkconfig
rm -fr build sdkconfig

# first we set idf to use linux as a target
idf.py --preview set-target linux

# we build the artifact
idf.py build

# we run the test
./build/test_bsdiff.elf

# build bsdiff and bspatch
gcc -O2 -DBSDIFF_EXECUTABLE -o esp32_bsdiff ../bsdiff.c
gcc -O2 -DBSPATCH_EXECUTABLE -o esp32_bspatch ../bspatch.c

# run a smoke test
./esp32_bsdiff ../bsdiff.c ../bspatch.c build/test_patch.bin
./esp32_bspatch ../bsdiff.c build/bspatch.c $(stat --printf="%s" ../bspatch.c) build/test_patch.bin

# compare files are identical
cmp --silent ../bspatch.c build/bspatch.c

