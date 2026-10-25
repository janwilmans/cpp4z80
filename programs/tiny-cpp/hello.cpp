/* hello_cpp.com: 34 bytes, the same code as tiny/hello.c.

     12  code: ld de,msg / ld c,9 / call 5 / ld hl,0 / ret
     22  the string "Hello from MSX-DOS!\r\n" with '$' stored in place of
         its terminating NUL (see std_print.h)

   The literal is written without the '$'; the constexpr copy that ends in
   '$' is made by the compiler, and nothing is built at run time.  */

#include "std_print.h"

int main ()
{
  std_print ("Hello from MSX-DOS!\r\n");
  return 0;
}
