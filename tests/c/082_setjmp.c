// setjmp / longjmp (newlib, libc/machine/z80/setjmp.S).
// expect: 0
#include <setjmp.h>

static jmp_buf env;
static int depth;

__attribute__ ((noipa)) void dive (int n)
{
  depth = n;
  if (n == 5)
    longjmp (env, n * 10);
  dive (n + 1);
}

int main (void)
{
  volatile int calls = 0;
  int r = setjmp (env);
  calls++;
  if (r == 0)
    dive (1);
  if (r != 50 || depth != 5 || calls != 2)
    return 1;
  if (setjmp (env) == 0)
    longjmp (env, 0);	/* setjmp then returns 1 */
  return 0;
}
