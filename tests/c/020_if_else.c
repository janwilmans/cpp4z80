// if/else on 8-bit and 16-bit values.
// expect: 3
__attribute__ ((noipa)) unsigned char max8 (unsigned char a, unsigned char b)
{
  if (a > b)
    return a;
  return b;
}

__attribute__ ((noipa)) int pick (int x)
{
  if (x == 1000)
    return 1;
  else
    return 2;
}

int main (void)
{
  return max8 (1, 0) + pick (1000) + pick (999) - 1;
}
