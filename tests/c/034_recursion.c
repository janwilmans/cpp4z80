// Recursive functions.
// expect: 0
__attribute__ ((noipa)) unsigned fib (unsigned char n)
{
  return n < 2 ? n : fib (n - 1) + fib (n - 2);
}

__attribute__ ((noipa)) unsigned gcd (unsigned a, unsigned b)
{
  return b == 0 ? a : gcd (b, a % b);
}

__attribute__ ((noipa)) int ackermann (int m, int n)
{
  if (m == 0)
    return n + 1;
  if (n == 0)
    return ackermann (m - 1, 1);
  return ackermann (m - 1, ackermann (m, n - 1));
}

int main (void)
{
  if (fib (15) != 610) return 1;
  if (gcd (1071, 462) != 21) return 2;
  if (ackermann (2, 3) != 9) return 3;
  return 0;
}
