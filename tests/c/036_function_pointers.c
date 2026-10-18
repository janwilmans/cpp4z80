// Indirect calls through function pointers.
// expect: 0
__attribute__ ((noipa)) int add (int a, int b) { return a + b; }
__attribute__ ((noipa)) int sub (int a, int b) { return a - b; }
__attribute__ ((noipa)) int mul (int a, int b) { return a * b; }

typedef int (*binop) (int, int);
binop ops[3] = { add, sub, mul };

__attribute__ ((noipa)) int apply (binop f, int a, int b)
{
  return f (a, b);
}

int main (void)
{
  if (apply (add, 3, 4) != 7) return 1;
  if (apply (sub, 3, 4) != -1) return 2;
  int r = 0;
  for (int i = 0; i < 3; i++)
    r += ops[i] (6, 3);
  if (r != 9 + 3 + 18) return 3;
  return 0;
}
