// Two 16-bit arguments in HL and DE, 16-bit addition.
// expect: 1234
__attribute__ ((noipa)) int add (int a, int b)
{
  return a + b;
}

int main (void)
{
  return add (1000, 234);
}
