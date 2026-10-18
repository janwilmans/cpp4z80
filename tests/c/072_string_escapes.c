// String literals containing quotes, backslashes and control characters
// (the z80 assembler needs these written as octal escapes).
// expect: 0
static const char s[] = "a\"b\\c\001\n\377";

int main (void)
{
  const unsigned char expect[] = { 'a', '"', 'b', '\\', 'c', 1, '\n', 255, 0 };
  for (unsigned i = 0; i < sizeof expect; i++)
    if ((unsigned char) s[i] != expect[i])
      return i + 1;
  return sizeof s == 9 ? 0 : 100;
}
