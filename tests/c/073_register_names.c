// C identifiers that are Z80 register names or condition codes ("i", "hl",
// "nz", ...) must work: C symbols get a "_" prefix in assembly.
// expect: 0
int i = 1, a = 2, hl = 3, sp = 4, ix = 5, r = 6, nz = 7, af = 8;

__attribute__ ((noipa)) int c (int b) { return b + 1; }

int main (void)
{
  return i + a + hl + sp + ix + r + nz + af + c (-1) - 36;
}
