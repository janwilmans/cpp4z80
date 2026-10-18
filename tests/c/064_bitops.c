// __builtin_clz/ctz/popcount/parity/ffs/clrsb on 16-bit int (libgcc
// lib1funcs.S) and on 32-bit long (libgcc2).
// expect: 0
__attribute__ ((noipa)) int clz (unsigned x) { return __builtin_clz (x); }
__attribute__ ((noipa)) int ctz (unsigned x) { return __builtin_ctz (x); }
__attribute__ ((noipa)) int pop (unsigned x) { return __builtin_popcount (x); }
__attribute__ ((noipa)) int par (unsigned x) { return __builtin_parity (x); }
__attribute__ ((noipa)) int ffs (int x) { return __builtin_ffs (x); }
__attribute__ ((noipa)) int clrsb (int x) { return __builtin_clrsb (x); }
__attribute__ ((noipa)) int clzl (unsigned long x) { return __builtin_clzl (x); }
__attribute__ ((noipa)) int popl (unsigned long x) { return __builtin_popcountl (x); }

int main (void)
{
  if (clz (1) != 15 || clz (0x8000) != 0 || clz (0x00f0) != 8) return 1;
  if (ctz (1) != 0 || ctz (0x8000) != 15 || ctz (0x0a00) != 9) return 2;
  if (pop (0) != 0 || pop (0xffff) != 16 || pop (0x1234) != 5) return 3;
  if (par (0x1234) != 1 || par (0x1233) != 0) return 4;
  if (ffs (0) != 0 || ffs (0x0100) != 9) return 5;
  if (clrsb (0) != 15 || clrsb (-1) != 15 || clrsb (1) != 14 || clrsb (-256) != 7) return 6;
  if (clzl (1) != 31 || popl (0xf0f0f0f0ul) != 16) return 7;
  return 0;
}
