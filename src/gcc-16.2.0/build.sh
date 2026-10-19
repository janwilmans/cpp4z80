#!/bin/bash
# Configure and build the z80-elf compiler in src/build-z80.
# The toolchain is installed into install/ (see scripts/build-toolchain.sh).
#
# CFLAGS/CXXFLAGS: the compiler itself is built without optimisation and
# with debug information, which makes it easy to debug the backend.  Use
# e.g. CFLAGS="-O2" CXXFLAGS="-O2" for a faster compiler.
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

rm -rf "$ROOT/src/build-z80"
mkdir -p "$ROOT/src/build-z80"
cd "$ROOT/src/build-z80"

../gcc-16.2.0/configure \
    --prefix="$ROOT/install" \
    --target=z80-elf \
    --enable-languages=c,c++ \
    --with-newlib \
    --disable-hosted-libstdcxx \
    --disable-libstdcxx-verbose \
    --disable-wchar_t \
    --disable-nls \
    --disable-werror \
    CFLAGS="${CFLAGS:--O0 -g3}" \
    CXXFLAGS="${CXXFLAGS:--O0 -g3}"

# build only gcc itself first; the libraries follow in
# scripts/build-toolchain.sh
make -j"$(nproc)" all-gcc
