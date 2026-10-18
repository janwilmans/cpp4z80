// Byte arguments on the stack take a 2-byte slot.
// expect: 15
__attribute__ ((noipa)) unsigned char sum5 (unsigned char a, unsigned char b, unsigned char c,
		    unsigned char d, unsigned char e)
{
  return a + b + c + d + e;
}

int main (void)
{
  return sum5 (1, 2, 3, 4, 5);
}
