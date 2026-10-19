// Variadic functions: all arguments go on the stack.
// expect: 0
#include <stdarg.h>

__attribute__ ((noipa)) long sum (int count, ...)
{
  va_list ap;
  long total = 0;
  va_start (ap, count);
  for (int i = 0; i < count; i++)
    {
      int kind = va_arg (ap, int);
      if (kind == 0)
	total += va_arg (ap, int);
      else
	total += va_arg (ap, long);
    }
  va_end (ap);
  return total;
}

__attribute__ ((noipa)) int first_char (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  int c = va_arg (ap, int);	/* char is promoted to int */
  va_end (ap);
  return c + *fmt;
}

int main (void)
{
  if (sum (3, 0, 1000, 1, 100000L, 0, -7) != 100993L) return 1;
  if (sum (0) != 0) return 2;
  if (first_char ("a", 'b') != 'a' + 'b') return 3;
  return 0;
}
