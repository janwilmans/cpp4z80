// String handling with char pointers.
// expect: 0
__attribute__ ((noipa)) unsigned my_strlen (const char *s)
{
  unsigned n = 0;
  while (*s++)
    n++;
  return n;
}

__attribute__ ((noipa)) void my_strcpy (char *d, const char *s)
{
  while ((*d++ = *s++))
    ;
}

__attribute__ ((noipa)) int my_strcmp (const char *a, const char *b)
{
  while (*a && *a == *b)
    a++, b++;
  return (unsigned char) *a - (unsigned char) *b;
}

__attribute__ ((noipa)) void reverse (char *s)
{
  char *e = s + my_strlen (s) - 1;
  while (s < e)
    {
      char t = *s;
      *s++ = *e;
      *e-- = t;
    }
}

int main (void)
{
  char buf[16];
  if (my_strlen ("hello") != 5) return 1;
  my_strcpy (buf, "Z80 rocks");
  if (my_strcmp (buf, "Z80 rocks") != 0) return 2;
  reverse (buf);
  if (my_strcmp (buf, "skcor 08Z") != 0) return 3;
  if (my_strcmp ("abc", "abd") >= 0) return 4;
  return 0;
}
