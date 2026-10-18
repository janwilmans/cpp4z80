// Zero-initialised globals live in .bss, which crt0 must clear (the test
// harness fills memory with garbage before loading the program).
// expect: 0
int counters[20];
static unsigned char flags[50];
long total;

int main (void)
{
  for (int i = 0; i < 20; i++)
    if (counters[i] != 0)
      return 1;
  for (int i = 0; i < 50; i++)
    if (flags[i] != 0)
      return 2;
  if (total != 0)
    return 3;
  return 0;
}
