// 32-bit long arithmetic: arguments on the stack, result in DE:HL.
// expect: 0
__attribute__ ((noipa)) long add (long a, long b) { return a + b; }
__attribute__ ((noipa)) long sub (long a, long b) { return a - b; }
__attribute__ ((noipa)) unsigned long band (unsigned long a, unsigned long b) { return a & b; }
__attribute__ ((noipa)) unsigned long bor (unsigned long a, unsigned long b) { return a | b; }
__attribute__ ((noipa)) long neg (long a) { return -a; }
__attribute__ ((noipa)) int lt (long a, long b) { return a < b; }
__attribute__ ((noipa)) int ltu (unsigned long a, unsigned long b) { return a < b; }
__attribute__ ((noipa)) int eq (long a, long b) { return a == b; }
__attribute__ ((noipa)) long widen (int x) { return x; }
__attribute__ ((noipa)) unsigned long uwiden (unsigned x) { return x; }
__attribute__ ((noipa)) int narrow (long x) { return (int) x; }

long global = 100000;

int main (void)
{
  if (add (100000, 200000) != 300000) return 1;
  if (add (-1, 1) != 0) return 2;
  if (add (0xffff, 1) != 0x10000) return 3;
  if (sub (0x10000, 1) != 0xffff) return 4;
  if (sub (5, 10) != -5) return 5;
  if (band (0x12345678ul, 0xff00ff00ul) != 0x12005600ul) return 6;
  if (bor (0x12340000ul, 0x5678ul) != 0x12345678ul) return 7;
  if (neg (123456) != -123456) return 8;
  if (!lt (-100000, 5) || lt (5, -100000)) return 9;
  if (!ltu (5, 0x80000000ul) || ltu (0x80000000ul, 5)) return 10;
  if (!eq (0x12345678, 0x12345678) || eq (0x12345678, 0x12345679)) return 11;
  if (widen (-2) != -2) return 12;
  if (uwiden (0xfffe) != 0xfffe) return 13;
  if (narrow (0x12345678) != 0x5678) return 14;
  global += 23456;
  if (global != 123456) return 15;
  return 0;
}
