// More than two arguments: the rest are pushed on the stack.
// expect: 4321
__attribute__ ((noipa)) int sum4 (int a, int b, int c, int d)
{
  return a + b + c + d;
}

int main (void)
{
  return sum4 (4000, 300, 20, 1);
}
