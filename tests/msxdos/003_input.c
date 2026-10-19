// MSX-DOS: reading stdin through BDOS function 10 (buffered line input,
// which echoes the typed line); end of input is Ctrl-Z.
// expect: 0
// input: 12 30
// input: hello world
// output: 12 30
// output: sum=42
// output: hello world
// output: line: hello world
// output:
// output: lines read: 2
#include <stdio.h>
#include <string.h>

int main (void)
{
  int a, b;
  char line[64];
  if (scanf ("%d %d", &a, &b) != 2)
    return 1;
  printf ("sum=%d\n", a + b);
  fgets (line, sizeof line, stdin);	/* rest of the first line */
  int n = 1;
  while (fgets (line, sizeof line, stdin))
    {
      line[strcspn (line, "\n")] = 0;
      printf ("line: %s\n", line);
      n++;
    }
  printf ("lines read: %d\n", n);
  return 0;
}
