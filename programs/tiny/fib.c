/* fib.com: 175 bytes.

      91  main: the add loop and the wrap-around test, with a, b and t in
          stack slots addressed through IX (3 bytes per access), plus 3
          calls to the print routines
      64  putu_: decimal printing.  Recursion plus a subtract-10 loop to get
          the quotient; no divide, so libgcc's __udivhi3 stays out
      16  __z80_enter / __z80_leave (frame setup)
       3  the string "\r\n$"
       1  padding

   putu_ is the one reusable piece: any program printing numbers pays about
   64 bytes for it.  Using the library's printf instead costs ~15 KB.  */

/* Print the Fibonacci numbers that fit in 16 bits. */
#include "tiny.h"

int main (void)
{
  unsigned a = 0, b = 1;

  for (;;)
    {
      putu_ (a);
      puts_ ("\r\n$");
      unsigned t = a + b;
      if (t < b)		/* wrapped around: b is the last one */
	{
	  putu_ (b);
	  puts_ ("\r\n$");
	  break;
	}
      a = b;
      b = t;
    }
  return 0;
}
