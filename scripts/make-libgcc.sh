#!/bin/bash
# Build libgcc for z80-elf with the freshly built compiler and show errors.
# Always rebuilds from scratch: the objects do not depend on the compiler,
# so an incremental build would keep code from an older backend.
cd "$(dirname "$0")/../src/build-z80" || exit 1
rm -rf z80-elf/libgcc
make -j8 all-target-libgcc > ../build-libgcc.log 2>&1
status=$?
grep -E "error:|Error [0-9]|internal compiler error|undefined reference" \
  ../build-libgcc.log | grep -v "Wno-error" | head -40
echo "make exit=$status"
exit $status
