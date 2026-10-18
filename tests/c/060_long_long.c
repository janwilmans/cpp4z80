// 64-bit long long arithmetic (libgcc2 routines).
// expect: 0
typedef long long i64;
typedef unsigned long long u64;

__attribute__ ((noipa)) i64 add (i64 a, i64 b) { return a + b; }
__attribute__ ((noipa)) i64 mul (i64 a, i64 b) { return a * b; }
__attribute__ ((noipa)) i64 div (i64 a, i64 b) { return a / b; }
__attribute__ ((noipa)) u64 umod (u64 a, u64 b) { return a % b; }
__attribute__ ((noipa)) u64 shl (u64 a, int n) { return a << n; }
__attribute__ ((noipa)) i64 sar (i64 a, int n) { return a >> n; }
__attribute__ ((noipa)) int cmp (i64 a, i64 b) { return a < b; }

int main (void)
{
  if (add (0xffffffffll, 1) != 0x100000000ll) return 1;
  if (mul (1000000ll, 1000000ll) != 1000000000000ll) return 2;
  if (mul (-3, 0x123456789ll) != -0x369d0369bll) return 3;
  if (div (1000000000000ll, 7) != 142857142857ll) return 4;
  if (div (-1000000000000ll, 1000) != -1000000000ll) return 5;
  if (umod (0xffffffffffffffffull, 10) != 5) return 6;
  if (shl (1, 40) != 0x10000000000ull) return 7;
  if (sar (-0x10000000000ll, 36) != -16) return 8;
  if (!cmp (-1, 0x100000000ll) || cmp (0x100000000ll, -1)) return 9;
  return 0;
}
