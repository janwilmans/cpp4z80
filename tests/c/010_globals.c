// Global variables live at absolute addresses.
// expect: 579
int counter = 123;
unsigned char byte = 0;

__attribute__ ((noipa)) void bump (int n)
{
  counter = counter + n;
  byte = byte + 1;
}

int main (void)
{
  bump (456);
  return counter + byte - 1;
}
