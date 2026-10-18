#!/bin/bash
# Incrementally rebuild the z80 cross compiler and show only real errors.
cd "$(dirname "$0")/../src/build-z80" || exit 1
make -j8 all-gcc > ../build-z80.log 2>&1
status=$?
grep -E "error:|Error [0-9]|undefined reference" ../build-z80.log | grep -v "Wno-error" | head -40
echo "make exit=$status"
exit $status
