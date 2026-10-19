#!/bin/bash
# Report the total code size (.text, bytes) of the benchmark test programs
# (listed in tests/benchmark.txt) per optimisation level, from the objects
# left by tests/run_tests.py.  The list is fixed so that the numbers stay
# comparable when tests are added.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SIZE="$ROOT/install/bin/z80-elf-size"
cd "$ROOT/work/test-out" || exit 1
for o in -O0 -O1 -O2 -Os; do
  total=0
  while read -r t; do
    name=$(basename "${t%.*}")
    case "$t" in c/*) dir="c-$name$o" ;; *) dir="c++-$name$o" ;; esac
    s=$("$SIZE" -A "$dir/$name.o" | awk '/^\.text/ {s += $2} END {print s + 0}')
    total=$((total + s))
  done < "$ROOT/tests/benchmark.txt"
  echo "$o: $total bytes"
done
