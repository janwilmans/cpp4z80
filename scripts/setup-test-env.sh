#!/bin/bash
# One-time setup of everything the test suite needs besides the compiler:
#   - z80-elf binutils (assembler/linker) installed into ./install
#   - a Python venv in ./work/venv with the `z80` emulator package
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if [ ! -x "$ROOT/install/bin/z80-elf-as" ]; then
  mkdir -p "$ROOT/src/build-binutils-z80"
  cd "$ROOT/src/build-binutils-z80"
  "$ROOT/src/binutils-2.45/configure" --prefix="$ROOT/install" \
    --target=z80-elf --disable-nls --disable-werror --disable-gprofng
  make -j"$(nproc)"
  make install
fi

if [ ! -x "$ROOT/work/venv/bin/python" ]; then
  python3 -m venv "$ROOT/work/venv"
  "$ROOT/work/venv/bin/pip" install z80
fi
echo "test environment ready"
