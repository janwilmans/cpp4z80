// double and long double are 32-bit (the same format as float).
// expect: 0
_Static_assert (sizeof (double) == 4 && sizeof (long double) == 4,
		"32-bit double");

__attribute__ ((noipa)) double dmul (double a, double b) { return a * b; }
__attribute__ ((noipa)) double dsub (double a, double b) { return a - b; }
__attribute__ ((noipa)) long to_long (double d) { return (long) d; }

int main (void)
{
  double x = dmul (1234.5, 1000.0);
  if (to_long (x) != 1234500) return 1;
  if (to_long (dsub (1e6, 1.0)) != 999999) return 2;
  double r = 1.0;
  for (int i = 0; i < 20; i++)
    r = r * 2.0;
  if (to_long (r) != 1048576) return 3;
  return 0;
}
