// switch statements (dense and sparse).
// expect: 0
__attribute__ ((noipa)) int classify (int c)
{
  switch (c)
    {
    case 0: return 10;
    case 1: return 11;
    case 2: return 12;
    case 3: return 13;
    case 4: return 14;
    case 5: return 15;
    case 1000: return 99;
    default: return -1;
    }
}

__attribute__ ((noipa)) unsigned char hexval (char c)
{
  switch (c)
    {
    case '0' ... '9': return c - '0';
    case 'a' ... 'f': return c - 'a' + 10;
    case 'A' ... 'F': return c - 'A' + 10;
    default: return 255;
    }
}

int main (void)
{
  for (int i = 0; i < 6; i++)
    if (classify (i) != 10 + i) return 1;
  if (classify (1000) != 99) return 2;
  if (classify (6) != -1 || classify (-1) != -1) return 3;
  if (hexval ('7') != 7 || hexval ('c') != 12 || hexval ('F') != 15) return 4;
  if (hexval ('x') != 255) return 5;
  return 0;
}
