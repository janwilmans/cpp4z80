#!/bin/bash
# Report the total code size (.text, bytes) of the test programs per
# optimisation level, from the objects left by tests/run_tests.py.
# Only the "benchmark" tests count (tests/c/0[0-5]*, tests/c/07[0-2]* and
# tests/cpp/*), so that numbers stay comparable when tests are added.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SIZE="$ROOT/install/bin/z80-elf-size"
cd "$ROOT/work/test-out" || exit 1
for o in -O0 -O1 -O2 -Os; do
  total=$(for f in c-0[0-5]*"$o"/*.o c-07[0-2]*"$o"/*.o c++-*"$o"/*.o; do
	    [ -e "$f" ] && "$SIZE" -A "$f"; done \
	  | awk '/^\.text/ {s += $2} END {print s + 0}')
  echo "$o: $total bytes"
done
