// A stack frame larger than the (ix+d) displacement range (-128..127).
// expect: 0
__attribute__ ((noipa)) int use (char *p, int n)
{
  int s = 0;
  for (int i = 0; i < n; i++)
    s += p[i];
  return s;
}

__attribute__ ((noipa)) int big (int k)
{
  char buf[300];
  int after = k;
  for (int i = 0; i < 300; i++)
    buf[i] = i & 7;
  return use (buf, 300) + after;
}

int main (void)
{
  if (big (5) != 37 * 28 + (0 + 1 + 2 + 3) + 5) return 1;
  return 0;
}
