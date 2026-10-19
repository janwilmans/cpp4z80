// MSX-DOS: console output through BDOS function 2, "\n" becomes CR LF.
// expect: 0
// output: Hello, MSX-DOS!
#include <stdio.h>

int main ()
{
  printf ("Hello, MSX-DOS!\n");
  return 0;
}
