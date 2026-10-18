// A few classic algorithms: insertion sort, binary search, CRC-16 and a
// sieve of Eratosthenes.
// expect: 0
__attribute__ ((noipa)) void sort (int *a, int n)
{
  for (int i = 1; i < n; i++)
    {
      int v = a[i], j = i - 1;
      while (j >= 0 && a[j] > v)
	{
	  a[j + 1] = a[j];
	  j--;
	}
      a[j + 1] = v;
    }
}

__attribute__ ((noipa)) int bsearch_int (const int *a, int n, int key)
{
  int lo = 0, hi = n - 1;
  while (lo <= hi)
    {
      int mid = (lo + hi) / 2;
      if (a[mid] == key)
	return mid;
      if (a[mid] < key)
	lo = mid + 1;
      else
	hi = mid - 1;
    }
  return -1;
}

__attribute__ ((noipa)) unsigned crc16_ccitt (const unsigned char *p,
					      unsigned n)
{
  unsigned crc = 0xffff;
  while (n--)
    {
      crc ^= (unsigned) *p++ << 8;
      for (unsigned char i = 0; i < 8; i++)
	crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
    }
  return crc;
}

__attribute__ ((noipa)) int count_primes (int limit)
{
  static unsigned char composite[200];
  int n = 0;
  for (int i = 2; i < limit; i++)
    {
      if (composite[i])
	continue;
      n++;
      for (int j = i * 2; j < limit; j += i)
	composite[j] = 1;
    }
  return n;
}

int main (void)
{
  int a[] = { 42, -5, 17, 1000, 3, 3, -300, 8 };
  sort (a, 8);
  for (int i = 1; i < 8; i++)
    if (a[i - 1] > a[i])
      return 1;
  if (bsearch_int (a, 8, 17) != 5 || bsearch_int (a, 8, 4) != -1) return 2;
  if (crc16_ccitt ((const unsigned char *) "123456789", 9) != 0x29b1) return 3;
  if (count_primes (200) != 46) return 4;
  return 0;
}
