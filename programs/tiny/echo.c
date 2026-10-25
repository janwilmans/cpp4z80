/* echo.com: 83 bytes.

     ~65  main: a loop over the command line bytes at 0x81.  Its counter
          lives in a stack slot addressed through IX, and every IX access
          is a 3-byte instruction.  Each putc_ is: ld e,(hl) / ld d,0 /
          ld c,2 / call 5.
      16  __z80_enter / __z80_leave (from libgcc), the shared prologue and
          epilogue the -Os code uses to set up the IX frame for that counter
       3  the string "\r\n$"

   Making the counter a register variable, or walking a pointer instead of
   indexing, would avoid the frame and its 16 + ~10 bytes.  */

/* Print the command line arguments. */
#include "tiny.h"

int main (void)
{
  for (unsigned char i = 0; i < CMDLINE_LEN; i++)
    putc_ (CMDLINE_TEXT[i]);
  puts_ ("\r\n$");
  return 0;
}
