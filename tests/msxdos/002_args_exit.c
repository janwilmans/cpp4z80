// MSX-DOS: command line arguments (from the command tail at 0x80) and the
// exit code (MSX-DOS 2 _TERM).
// expect: 3
// args: ONE TWO
// output: argc=3
// output: [ONE]
// output: [TWO]
#include <stdio.h>

int main (int argc, char **argv)
{
  printf ("argc=%d\n", argc);
  for (int i = 1; i < argc; i++)
    printf ("[%s]\n", argv[i]);
  return argc;
}
