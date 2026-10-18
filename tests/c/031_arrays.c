// Arrays on the stack and in global memory, indexed access.
// expect: 0
int table[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
unsigned char bytes[5] = { 10, 20, 30, 40, 50 };

__attribute__ ((noipa)) int sum (const int *p, int n)
{
  int s = 0;
  for (int i = 0; i < n; i++)
    s += p[i];
  return s;
}

__attribute__ ((noipa)) unsigned bsum (const unsigned char *p, unsigned char n)
{
  unsigned s = 0;
  while (n--)
    s += *p++;
  return s;
}

int main (void)
{
  int local[4];
  for (int i = 0; i < 4; i++)
    local[i] = i * i;
  if (sum (local, 4) != 14) return 1;
  if (sum (table, 8) != 36) return 2;
  if (bsum (bytes, 5) != 150) return 3;
  table[3] = 100;
  if (sum (table, 8) != 132) return 4;
  return 0;
}
