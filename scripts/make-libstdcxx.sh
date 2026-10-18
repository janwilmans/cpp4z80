#!/bin/bash
# Build the freestanding C++ library (libstdc++-v3 configured with
# --disable-hosted-libstdcxx) for z80-elf and show errors.
cd "$(dirname "$0")/../src/build-z80" || exit 1
rm -rf z80-elf/libstdc++-v3
make -j8 all-target-libstdc++-v3 > ../build-libstdcxx.log 2>&1
status=$?
grep -E "error:|Error [0-9]|internal compiler error" ../build-libstdcxx.log \
  | grep -v "Wno-error" | head -40
echo "make exit=$status"
exit $status
