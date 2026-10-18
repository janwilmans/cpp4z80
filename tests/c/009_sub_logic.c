// 16-bit subtraction and 8-bit logic operations.
// expect: 1000
__attribute__ ((noipa)) int sub (int a, int b)
{
  return a - b;
}

__attribute__ ((noipa)) unsigned char logic (unsigned char a, unsigned char b)
{
  return (a & 0xf0) | (b ^ 0x0f);
}

int main (void)
{
  return sub (1234, 234) + logic (0x12, 0x0f) - 0x10;
}
