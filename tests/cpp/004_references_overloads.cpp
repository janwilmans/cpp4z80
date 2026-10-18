// References, overloading, namespaces and constexpr.
// expect: 0
namespace z80
{
  constexpr int square (int x) { return x * x; }

  __attribute__ ((noipa)) void swap (int &a, int &b)
  {
    int t = a;
    a = b;
    b = t;
  }

  __attribute__ ((noipa)) int describe (int) { return 1; }
  __attribute__ ((noipa)) int describe (unsigned char) { return 2; }
  __attribute__ ((noipa)) int describe (const char *) { return 3; }
}

static_assert (z80::square (12) == 144, "constexpr evaluation");
static_assert (sizeof (int) == 2 && sizeof (long) == 4 && sizeof (void *) == 2,
	       "Z80 type sizes");

int main ()
{
  int a = 1, b = 2;
  z80::swap (a, b);
  if (a != 2 || b != 1) return 1;
  unsigned char c = 7;
  if (z80::describe (5) + z80::describe (c) + z80::describe ("x") != 6)
    return 2;
  return 0;
}
