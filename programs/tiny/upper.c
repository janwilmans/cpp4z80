/* upper.com: 153 bytes.

     ~125  main: two BDOS prints, the BDOS read-line call, and the loop that
           upper-cases each byte (add a,0x9f / cp 0x19 ... for the
           'a'..'z' test).  Much of it is IX-relative addressing of the
           82-byte buffer on the stack: every access is 3 bytes, and
           computing the buffer's address (ix-82) takes 10.
      16   __z80_enter / __z80_leave (frame setup for the buffer)
      10   the strings "text? $" (7) and "\r\n$" (3, shared by both uses)
       2   padding

   A buffer at a fixed address instead of on the stack (e.g. 0x80, the
   command line area) would make the addressing much shorter.  */

/* Read a line, print it in upper case. */
#include "tiny.h"

int main (void)
{
  unsigned char buf[2 + 80];

  buf[0] = 80;

  puts_ ("text? $");
  readline_ (buf);
  puts_ ("\r\n$");
  for (unsigned char i = 0; i < buf[1]; i++)
    {
      char c = buf[2 + i];
      putc_ (c >= 'a' && c <= 'z' ? c - 32 : c);
    }
  puts_ ("\r\n$");
  return 0;
}
