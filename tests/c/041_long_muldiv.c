// 32-bit multiply, divide and shifts (runtime library routines).
// expect: 0
__attribute__ ((noipa)) long mul (long a, long b) { return a * b; }
__attribute__ ((noipa)) long div (long a, long b) { return a / b; }
__attribute__ ((noipa)) long mod (long a, long b) { return a % b; }
__attribute__ ((noipa)) unsigned long udiv (unsigned long a, unsigned long b) { return a / b; }
__attribute__ ((noipa)) unsigned long umod (unsigned long a, unsigned long b) { return a % b; }
__attribute__ ((noipa)) long shl (long a, unsigned char n) { return a << n; }
__attribute__ ((noipa)) long sar (long a, unsigned char n) { return a >> n; }
__attribute__ ((noipa)) unsigned long shr (unsigned long a, unsigned char n) { return a >> n; }

int main (void)
{
  if (mul (1000, 1000) != 1000000) return 1;
  if (mul (-12345, 6789) != -83810205) return 2;
  if (mul (0x10000, 0x10000) != 0) return 3;
  if (div (1000000, 7) != 142857) return 4;
  if (div (-1000000, 7) != -142857) return 5;
  if (div (1000000, -7) != -142857) return 6;
  if (mod (1000000, 7) != 1) return 7;
  if (mod (-1000000, 7) != -1) return 8;
  if (udiv (4000000000ul, 3) != 1333333333ul) return 9;
  if (umod (4000000000ul, 3) != 1) return 10;
  if (udiv (0xffffffffu, 0x80000001ul) != 1) return 11;
  if (umod (0xffffffffu, 0x80000001ul) != 0x7ffffffeul) return 12;
  if (shl (1, 31) != (long) 0x80000000ul) return 13;
  if (shl (0x12345678, 4) != 0x23456780) return 14;
  if (sar (-0x10000000l, 8) != -0x100000l) return 15;
  if (shr (0x80000000ul, 31) != 1) return 16;
  if (shl (5, 0) != 5) return 17;
  return 0;
}
