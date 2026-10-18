// Structures: member access through pointers and by value in memory.
// expect: 0
struct point { int x, y; };
struct item { unsigned char tag; int value; struct point pos; };

struct item items[3];

__attribute__ ((noipa)) void move (struct point *p, int dx, int dy)
{
  p->x += dx;
  p->y += dy;
}

__attribute__ ((noipa)) int total (struct item *it, int n)
{
  int t = 0;
  for (int i = 0; i < n; i++)
    t += it[i].value + it[i].pos.x + it[i].pos.y + it[i].tag;
  return t;
}

int main (void)
{
  struct point p = { 10, 20 };
  move (&p, 5, -5);
  if (p.x != 15 || p.y != 15) return 1;
  for (int i = 0; i < 3; i++)
    {
      items[i].tag = i;
      items[i].value = 100 * i;
      items[i].pos = p;
    }
  if (total (items, 3) != 300 + 90 + 3) return 2;
  return 0;
}
