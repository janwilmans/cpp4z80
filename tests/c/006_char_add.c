// 8-bit arithmetic goes through the accumulator.
// expect: 30
__attribute__ ((noipa)) unsigned char add (unsigned char a, unsigned char b)
{
  return a + b;
}

int main (void)
{
  return add (10, 20);
}
