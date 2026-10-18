// Single precision floating point (soft float from libgcc, fp-bit).
// expect: 0
__attribute__ ((noipa)) float fadd (float a, float b) { return a + b; }
__attribute__ ((noipa)) float fmul (float a, float b) { return a * b; }
__attribute__ ((noipa)) float fdiv (float a, float b) { return a / b; }
__attribute__ ((noipa)) int to_int (float f) { return (int) f; }
__attribute__ ((noipa)) float from_int (int i) { return i; }
__attribute__ ((noipa)) int flt (float a, float b) { return a < b; }

__attribute__ ((noipa)) float poly (float x)
{
  return ((2.0f * x - 3.0f) * x + 0.5f) * x;
}

int main (void)
{
  if (fadd (1.5f, 2.25f) != 3.75f) return 1;
  if (fmul (-4.0f, 2.5f) != -10.0f) return 2;
  if (fdiv (1.0f, 4.0f) != 0.25f) return 3;
  if (to_int (123.9f) != 123 || to_int (-7.5f) != -7) return 4;
  if (from_int (-300) != -300.0f) return 5;
  if (!flt (-1.0f, 0.5f) || flt (2.0f, 2.0f)) return 6;
  if (to_int (poly (3.0f) * 100.0f) != 2850) return 7;
  float s = 0;
  for (int i = 1; i <= 10; i++)
    s += 1.0f / i;
  if (to_int (s * 1000.0f) != 2928) return 8;
  return 0;
}
