#!/bin/bash
# Install the z80-elf compiler and libgcc into ./install.
cd "$(dirname "$0")/../src/build-z80" || exit 1
make install-gcc install-target-libgcc > ../install-gcc.log 2>&1
status=$?
grep -E "error|Error [0-9]" ../install-gcc.log | head -20
echo "install exit=$status"
exit $status
