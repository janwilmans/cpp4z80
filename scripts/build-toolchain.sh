#!/bin/bash
# Build and install the complete z80-elf toolchain into ./install, in
# dependency order: compiler, libgcc, newlib (C library), freestanding
# libstdc++.  Assumes src/build-z80 is configured (src/gcc-16.2.0/build.sh)
# and binutils are installed (scripts/setup-test-env.sh).
set -e
cd "$(dirname "$0")"
./make-gcc.sh
./make-libgcc.sh
./install-gcc.sh
./build-newlib.sh
./make-libstdcxx.sh
cd ../src/build-z80
make install-target-libstdc++-v3 > ../install-libstdcxx.log 2>&1
echo "toolchain installed in $(cd ../../install && pwd)"
