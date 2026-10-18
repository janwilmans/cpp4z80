// Regression: GCC reuses a return value still in HL as the argument of the
// next call, so HL is live across the compare-and-branch.  This used to
// make LRA loop forever when compares required their operand in HL.
// expect: 0
__attribute__ ((noipa)) int classify (int c) { return c > 5 || c < 0 ? -1 : c; }

int main (void)
{
  if (classify (6) != -1 || classify (-1) != -1)
    return 3;
  return 0;
}
