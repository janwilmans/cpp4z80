/* hello_printf.com: 15866 bytes.

   The same stdio machinery as hello_puts.cpp (13232 bytes, see there),
   plus the formatter: __printf_i 2.7 KB (integer conversions),
   __vfprintf_r 1.3 KB, __printf_common 1 KB, __swbuf_r 0.6 KB, and more.
   That is ~2.6 KB for a single "%d".  Printing a number with the
   subtract-10 loop in tiny/tiny.h costs 64 bytes.  */

#include <stdio.h>

int main (int argc, char **argv)
{
  printf ("Hello from MSX-DOS! %d argument(s)\n", argc - 1);
  return 0;
}
