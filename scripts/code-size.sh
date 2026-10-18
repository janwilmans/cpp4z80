#!/bin/bash
# Report the total code size (.text, bytes) of all test programs per
# optimisation level, from the objects left by tests/run_tests.py.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SIZE="$ROOT/install/bin/z80-elf-size"
cd "$ROOT/work/test-out" || exit 1
for o in -O0 -O1 -O2 -Os; do
  total=$(for f in *"$o"/*.o; do "$SIZE" -A "$f"; done \
	  | awk '/^\.text/ {s += $2} END {print s + 0}')
  echo "$o: $total bytes"
done
