// newlib: string functions, qsort, malloc/free, atoi and snprintf.
// expect: 0
// output: sorted: 1 2 3 5 8 13
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmp (const void *a, const void *b)
{
  return *(const int *) a - *(const int *) b;
}

int main (void)
{
  char buf[32];
  strcpy (buf, "hello");
  strcat (buf, " world");
  if (strlen (buf) != 11 || strcmp (buf, "hello world") != 0) return 1;
  if (strchr (buf, 'w') != buf + 6 || memcmp (buf, "hello", 5) != 0) return 2;

  int v[] = { 8, 3, 13, 1, 5, 2 };
  qsort (v, 6, sizeof v[0], cmp);
  printf ("sorted:");
  for (int i = 0; i < 6; i++)
    printf (" %d", v[i]);
  printf ("\n");

  int *p = malloc (100 * sizeof (int));
  if (!p) return 3;
  for (int i = 0; i < 100; i++)
    p[i] = i;
  long sum = 0;
  for (int i = 0; i < 100; i++)
    sum += p[i];
  free (p);
  if (sum != 4950) return 4;

  if (atoi ("-1234") != -1234) return 5;
  snprintf (buf, sizeof buf, "%04d|%-3s|", 42, "ab");
  if (strcmp (buf, "0042|ab |") != 0) return 6;
  return 0;
}
