/* hello_udl.com: 34 bytes, identical code to hello_cpp.com and tiny/hello.c.

     12  code: ld de,msg / ld c,9 / call 5 / ld hl,0 / ret
     22  the string "Hello from MSX-DOS!\r\n" with '$' in place of its NUL

   Same result as hello.cpp, but std_print is a real function and there is
   no macro; the price is the _s suffix on the literal.  */

#include "std_print_udl.h"

int main ()
{
  std_print ("Hello from MSX-DOS!\r\n"_s);
  return 0;
}
