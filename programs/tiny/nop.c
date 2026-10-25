/* nop.com: 4 bytes.

   ld hl,0 (3 bytes, the return value 0) + ret (1 byte).  That is the whole
   program: with no start-up code nothing else is needed.  (A normal
   program built with -specs=msxdos.specs starts at about 0.4 KB.)  */

/* The smallest possible program: it does nothing. */
int main (void)
{
  return 0;
}
