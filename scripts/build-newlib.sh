#!/bin/bash
# Build newlib (C library) for z80-elf with the installed compiler and
# install it into ./install.  Run scripts/install-gcc.sh first.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export PATH="$ROOT/install/bin:$PATH"
BUILD="$ROOT/src/build-newlib-z80"
rm -rf "$BUILD"
mkdir -p "$BUILD"
cd "$BUILD" || exit 1
"$ROOT/src/newlib-4.6.0.20260123/configure" \
  --prefix="$ROOT/install" \
  --target=z80-elf \
  --disable-newlib-supplied-syscalls \
  --enable-newlib-nano-formatted-io \
  --disable-newlib-wide-orient \
  --enable-newlib-nano-malloc \
  --enable-newlib-reent-small \
  --enable-lite-exit \
  --enable-newlib-global-atexit \
  --disable-newlib-multithread \
  --disable-newlib-io-float \
  --disable-nls \
  > configure.log 2>&1 || { tail -20 configure.log; exit 1; }
make -j8 > make.log 2>&1
status=$?
grep -E "error:|Error [0-9]|internal compiler error" make.log | head -40
[ $status -eq 0 ] && make install > install.log 2>&1
echo "newlib exit=$status"
exit $status
