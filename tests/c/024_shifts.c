// Constant and variable shifts, 8 and 16 bit, logical and arithmetic.
// expect: 0
__attribute__ ((noipa)) unsigned shl (unsigned x, unsigned char n) { return x << n; }
__attribute__ ((noipa)) unsigned shr (unsigned x, unsigned char n) { return x >> n; }
__attribute__ ((noipa)) int sar (int x, unsigned char n) { return x >> n; }
__attribute__ ((noipa)) unsigned char shl8 (unsigned char x, unsigned char n)
{
  return (unsigned char) (x << n);
}
__attribute__ ((noipa)) unsigned shl3 (unsigned x) { return x << 3; }
__attribute__ ((noipa)) unsigned shr9 (unsigned x) { return x >> 9; }
__attribute__ ((noipa)) int sar10 (int x) { return x >> 10; }
__attribute__ ((noipa)) int sar4 (int x) { return x >> 4; }

int main (void)
{
  if (shl (0x1234, 4) != 0x2340) return 1;
  if (shr (0x8234, 4) != 0x0823) return 2;
  if (sar (-0x7dcc, 4) != -0x7dd) return 3;
  if (shl8 (0x81, 1) != 0x02) return 4;
  if (shl (1, 0) != 1) return 5;
  if (shl3 (0x1001) != 0x8008) return 6;
  if (shr9 (0xfe00) != 0x7f) return 7;
  if (sar10 (-1024) != -1) return 8;
  if (sar4 (-32) != -2) return 9;
  if (sar10 (0x7c00) != 0x1f) return 10;
  return 0;
}
