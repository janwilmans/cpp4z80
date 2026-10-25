/* countdown.com: 153 bytes.

      61  main: Countdown<10>::run () is inlined, as is putc_.  The loop
          counter is in a stack slot.  Output is putu_ (i), ' ' and then
          the string.
      64  putu_: decimal printing (see fib.c)
      16  __z80_enter / __z80_leave
      12  the string "liftoff!\r\n$"

   The class and the template add nothing: the compiler folds them away at
   -Os, so this is the same code a plain C loop would give.  Features that
   would cost bytes: exceptions, RTTI, virtual functions with a vtable,
   static objects with constructors (those need start-up code to run them),
   and anything from the C++ library.  */

/* C++ costs nothing extra when it is not used: a class and a template
   here compile to the same code as plain C. */
#include "tiny.h"

template <unsigned N>
struct Countdown
{
  void run () const
  {
    for (unsigned i = N; i > 0; i--)
      {
	putu_ (i);
	putc_ (' ');
      }
    puts_ ("liftoff!\r\n$");
  }
};

int main ()
{
  Countdown<10> ().run ();
  return 0;
}
