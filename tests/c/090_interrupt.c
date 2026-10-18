// Interrupt handlers preserve all registers and return with "ei / reti";
// naked functions have no prologue/epilogue.  The handler is called from
// inline assembly with known values in BC, DE and HL.
// expect: 0
volatile unsigned ticks;
volatile unsigned char work[4];

__attribute__ ((interrupt)) void isr (void)
{
  ticks++;
  for (unsigned char i = 0; i < 4; i++)
    work[i] = work[i] * 3 + i;
}

/* Returns 42 in HL, entirely in assembly.  */
__attribute__ ((naked)) int answer (void)
{
  __asm__ volatile ("ld hl,42\n\tret");
}

int main (void)
{
  unsigned bc, de, hl;
  for (int n = 0; n < 3; n++)
    {
      __asm__ volatile ("ld bc,0x1234\n\tld de,0x5678\n\tld hl,0x9abc\n\t"
			"call isr"
			: "=b" (bc), "=d" (de), "=h" (hl)
			:
			: "a", "iyl", "iyh", "memory");
      if (bc != 0x1234 || de != 0x5678 || hl != 0x9abc)
	return 1;
    }
  if (ticks != 3)
    return 2;
  if (answer () != 42)
    return 3;
  return 0;
}
