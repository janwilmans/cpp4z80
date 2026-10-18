#!/bin/bash
# Build the autotools versions that newlib's generated files require
# (autoconf 2.69, automake 1.15.1) into work/autotools.  Only needed when
# changing newlib's configure.ac / Makefile.am / Makefile.inc files; then
# run "autoreconf" in src/newlib-*/newlib with work/autotools/bin on PATH.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
P="$ROOT/work/autotools"
mkdir -p "$ROOT/work/autotools-src"
cd "$ROOT/work/autotools-src"
for t in autoconf/autoconf-2.69 automake/automake-1.15.1; do
  f="$(basename $t).tar.xz"
  [ -f "$f" ] || curl -sLO "https://ftp.gnu.org/gnu/$t.tar.xz"
  tar xf "$f"
done
export PATH="$P/bin:$PATH"
(cd autoconf-2.69 && ./configure --prefix="$P" && make && make install)
(cd automake-1.15.1 && ./configure --prefix="$P" && make && make install)
