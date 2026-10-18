// Many live values at once force spilling.
// expect: 0
__attribute__ ((noipa)) int mix (int a, int b, int c, int d, int e, int f)
{
  int t1 = a + b, t2 = b + c, t3 = c + d, t4 = d + e, t5 = e + f, t6 = f + a;
  int u1 = t1 * 3, u2 = t2 - t3, u3 = t4 + t5 + t6;
  return t1 + t2 + t3 + t4 + t5 + t6 + u1 + u2 + u3;
}

__attribute__ ((noipa)) unsigned char bytes (unsigned char a, unsigned char b,
					     unsigned char c, unsigned char d)
{
  unsigned char x = a ^ b, y = b + c, z = c & d, w = d | a;
  return (x + y) ^ (z - w) ^ (a + b + c + d);
}

int main (void)
{
  if (mix (1, 2, 3, 4, 5, 6) != 3 + 5 + 7 + 9 + 11 + 7 + 9 + (5 - 7) + 27)
    return 1;
  unsigned char x = 1 ^ 2, y = 2 + 3, z = 3 & 4, w = 4 | 1;
  if (bytes (1, 2, 3, 4) != (unsigned char) ((x + y) ^ (z - w) ^ 10))
    return 2;
  return 0;
}
