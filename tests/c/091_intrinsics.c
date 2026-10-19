// <z80intrin.h>: I/O ports, interrupt control.  The test machine prints
// what is written to port 1; reading port 2 returns 0x42, other ports the
// high byte of the 16-bit port address.
// expect: 0
// output: Hi!
#include <z80intrin.h>

int main (void)
{
  z80_di ();
  Z80_OUT (1, 'H');
  z80_out (1, 'i');
  z80_out (0x1201, '!');
  Z80_OUT (1, '\n');
  if (Z80_IN (2) != 0x42) return 1;
  if (z80_in (2) != 0x42) return 2;
  if (z80_in (0x3456) != 0x34) return 3;
  z80_im (1);
  z80_nop ();
  z80_ei ();
  return 0;
}
