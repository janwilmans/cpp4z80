// Struct assignment, struct arguments and struct return values.
// expect: 0
struct big { int a[10]; char tag; };
struct pair { int x, y; };

__attribute__ ((noipa)) struct pair make_pair (int x, int y)
{
  struct pair p = { x, y };
  return p;
}

__attribute__ ((noipa)) int sum_big (struct big b)
{
  int s = b.tag;
  for (int i = 0; i < 10; i++)
    s += b.a[i];
  return s;
}

__attribute__ ((noipa)) struct big make_big (int base)
{
  struct big b;
  for (int i = 0; i < 10; i++)
    b.a[i] = base + i;
  b.tag = 1;
  return b;
}

struct big global_big;

int main (void)
{
  struct pair p = make_pair (3, 4);
  if (p.x != 3 || p.y != 4) return 1;
  struct big b = make_big (100);
  if (sum_big (b) != 1046) return 2;
  global_big = b;
  global_big.a[0] = 0;
  if (sum_big (global_big) != 946 || b.a[0] != 100) return 3;
  return 0;
}
