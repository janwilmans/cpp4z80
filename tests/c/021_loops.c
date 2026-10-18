// for and while loops.
// expect: 5050
__attribute__ ((noipa)) int sum_to (int n)
{
  int s = 0;
  for (int i = 1; i <= n; i++)
    s += i;
  return s;
}

__attribute__ ((noipa)) unsigned char count_bits (unsigned char x)
{
  unsigned char n = 0;
  while (x)
    {
      n += x & 1;
      x >>= 1;
    }
  return n;
}

int main (void)
{
  if (count_bits (0xb5) != 5)
    return 1;
  return sum_to (100);
}
