// newlib: printf through the system layer (tests/support/sys.c), which
// writes to I/O port 1.
// expect: 0
// output: Hello, Z80!
// output: 42 -7 ff 65535 123456
// output: [  abc] [x]
#include <stdio.h>

int main (void)
{
  printf ("Hello, Z80!\n");
  printf ("%d %d %x %u %ld\n", 42, -7, 255, 65535u, 123456L);
  printf ("[%5s] [%c]\n", "abc", 'x');
  return 0;
}
