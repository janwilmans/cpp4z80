// Bitfields, volatile accesses and unions.
// expect: 0
struct flags
{
  unsigned ready : 1;
  unsigned mode : 3;
  unsigned count : 5;
  unsigned high : 7;
};

union word
{
  unsigned w;
  unsigned char b[2];
};

volatile unsigned char port;

__attribute__ ((noipa)) unsigned pack (unsigned mode, unsigned count)
{
  struct flags f = { 0 };
  f.ready = 1;
  f.mode = mode;
  f.count = count;
  f.high = 0x55;
  return f.ready + f.mode * 10 + f.count * 100 + (f.high == 0x55);
}

int main (void)
{
  if (pack (5, 17) != 1 + 50 + 1700 + 1) return 1;
  union word u;
  u.w = 0x1234;
  if (u.b[0] != 0x34 || u.b[1] != 0x12) return 2;
  for (int i = 0; i < 3; i++)
    port = port + 2;
  if (port != 6) return 3;
  return 0;
}
