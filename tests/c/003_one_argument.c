// A single 8-bit argument (passed in L) is returned unchanged.
// expect: 7
__attribute__ ((noipa)) unsigned char example (unsigned char x)
{
  return x;
}

int main (void)
{
  return example (7);
}
