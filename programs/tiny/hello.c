/* hello.com: 35 bytes.

     12  code: ld de,msg / ld c,9 / call 5 (the BDOS print-string call) /
         ld hl,0 / ret
     22  the string "Hello from MSX-DOS!\r\n$": 19 characters, CR LF, and
         the '$' that ends a BDOS function 9 string
      1  alignment padding

   The text is the largest part.  Compare newlib's puts () of the same
   text, 13232 bytes (newlib/hello_puts.cpp).  */

#include "tiny.h"

int main (void)
{
  puts_ ("Hello from MSX-DOS!\r\n$");
  return 0;
}
