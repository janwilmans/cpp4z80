// Sign and zero extension from 8 to 16 bits.
// expect: 0
__attribute__ ((noipa)) int sext (signed char c) { return c; }
__attribute__ ((noipa)) unsigned zext (unsigned char c) { return c; }

int main (void)
{
  if (sext (-5) != -5)
    return 1;
  if (sext (100) != 100)
    return 2;
  if (zext (200) != 200)
    return 3;
  if (sext ((signed char) 0x80) != -128)
    return 4;
  return 0;
}
