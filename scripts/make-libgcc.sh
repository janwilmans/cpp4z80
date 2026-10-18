#!/bin/bash
# Build libgcc for z80-elf with the freshly built compiler and show errors.
cd "$(dirname "$0")/../src/build-z80" || exit 1
make -j8 all-target-libgcc > ../build-libgcc.log 2>&1
status=$?
grep -E "error:|Error [0-9]|internal compiler error|undefined reference" \
  ../build-libgcc.log | grep -v "Wno-error" | head -40
echo "make exit=$status"
exit $status
