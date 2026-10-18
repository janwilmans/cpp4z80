// A local variable (lives in a stack slot at -O0).
// expect: 42
__attribute__ ((noipa)) unsigned char example (void)
{
  unsigned char x = 42;
  return x;
}

int main (void)
{
  return example ();
}
