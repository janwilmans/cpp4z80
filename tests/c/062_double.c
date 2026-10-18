// Double precision floating point (64-bit soft float, returned in memory).
// expect: 0
__attribute__ ((noipa)) double dmul (double a, double b) { return a * b; }
__attribute__ ((noipa)) double dsub (double a, double b) { return a - b; }
__attribute__ ((noipa)) long to_long (double d) { return (long) d; }

int main (void)
{
  double x = dmul (1234.5, 1000.0);
  if (to_long (x) != 1234500) return 1;
  if (to_long (dsub (1e9, 1.0)) != 999999999) return 2;
  double r = 1.0;
  for (int i = 0; i < 20; i++)
    r = r * 2.0;
  if (to_long (r) != 1048576) return 3;
  return 0;
}
