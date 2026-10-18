// An 8-bit function returning a constant, called from main.
// expect: 42
__attribute__ ((noipa)) unsigned char example (void)
{
  return 42;
}

int main (void)
{
  return example ();
}
