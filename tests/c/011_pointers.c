// Access through pointers uses (hl).
// expect: 300
__attribute__ ((noipa)) void store (int *p, int v)
{
  *p = v;
}

__attribute__ ((noipa)) int load (int *p)
{
  return *p;
}

int main (void)
{
  int x = 0;
  store (&x, 300);
  return load (&x);
}
