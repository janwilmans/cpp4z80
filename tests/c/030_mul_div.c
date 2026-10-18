// Multiplication and division use the runtime library (lib1funcs.S).
// expect: 0
__attribute__ ((noipa)) int mul (int a, int b) { return a * b; }
__attribute__ ((noipa)) unsigned char mul8 (unsigned char a, unsigned char b) { return a * b; }
__attribute__ ((noipa)) int div (int a, int b) { return a / b; }
__attribute__ ((noipa)) int mod (int a, int b) { return a % b; }
__attribute__ ((noipa)) unsigned udiv (unsigned a, unsigned b) { return a / b; }
__attribute__ ((noipa)) unsigned umod (unsigned a, unsigned b) { return a % b; }
__attribute__ ((noipa)) unsigned char udiv8 (unsigned char a, unsigned char b) { return a / b; }
__attribute__ ((noipa)) unsigned char umod8 (unsigned char a, unsigned char b) { return a % b; }
__attribute__ ((noipa)) int times10 (int a) { return a * 10; }

int main (void)
{
  if (mul (123, 45) != 5535) return 1;
  if (mul (-7, 6) != -42) return 2;
  if (mul (-300, -100) != 30000) return 3;
  if (mul8 (13, 17) != (unsigned char) 221) return 4;
  if (mul8 (200, 3) != (unsigned char) 600) return 5;
  if (div (1000, 7) != 142) return 6;
  if (div (-1000, 7) != -142) return 7;
  if (div (1000, -7) != -142) return 8;
  if (div (-1000, -7) != 142) return 9;
  if (mod (1000, 7) != 6) return 10;
  if (mod (-1000, 7) != -6) return 11;
  if (mod (1000, -7) != 6) return 12;
  if (udiv (60000u, 7) != 8571) return 13;
  if (umod (60000u, 7) != 3) return 14;
  if (udiv (65535u, 40000u) != 1) return 15;
  if (umod (65535u, 40000u) != 25535) return 16;
  if (udiv8 (250, 7) != 35) return 17;
  if (umod8 (250, 7) != 5) return 18;
  if (times10 (321) != 3210) return 19;
  if (div (-32768, 1) != -32768) return 20;
  return 0;
}
