// Every comparison operator, signed and unsigned, 8 and 16 bit, including
// the cases where the signed subtraction overflows.
// expect: 0
#define CMP_FUNCS(T, NAME)						\
  __attribute__ ((noipa)) int NAME##_lt (T a, T b) { return a < b; }	\
  __attribute__ ((noipa)) int NAME##_le (T a, T b) { return a <= b; }	\
  __attribute__ ((noipa)) int NAME##_gt (T a, T b) { return a > b; }	\
  __attribute__ ((noipa)) int NAME##_ge (T a, T b) { return a >= b; }	\
  __attribute__ ((noipa)) int NAME##_eq (T a, T b) { return a == b; }	\
  __attribute__ ((noipa)) int NAME##_ne (T a, T b) { return a != b; }	\
  static int NAME##_check (T a, T b)					\
  {									\
    int r = 0;								\
    r |= NAME##_lt (a, b) != (a < b);					\
    r |= NAME##_le (a, b) != (a <= b);					\
    r |= NAME##_gt (a, b) != (a > b);					\
    r |= NAME##_ge (a, b) != (a >= b);					\
    r |= NAME##_eq (a, b) != (a == b);					\
    r |= NAME##_ne (a, b) != (a != b);					\
    return r;								\
  }

CMP_FUNCS (signed char, s8)
CMP_FUNCS (unsigned char, u8)
CMP_FUNCS (int, s16)
CMP_FUNCS (unsigned int, u16)

static const int v16[] = { 0, 1, -1, 2, 100, -100, 32767, -32768, 32766,
			   -32767, 255, 256 };
static const signed char v8[] = { 0, 1, -1, 2, 100, -100, 127, -128, 126,
				  -127 };

int main (void)
{
  int fails = 0;
  for (unsigned i = 0; i < sizeof v16 / sizeof v16[0]; i++)
    for (unsigned j = 0; j < sizeof v16 / sizeof v16[0]; j++)
      {
	fails += s16_check (v16[i], v16[j]);
	fails += u16_check (v16[i], v16[j]);
      }
  for (unsigned i = 0; i < sizeof v8; i++)
    for (unsigned j = 0; j < sizeof v8; j++)
      {
	fails += s8_check (v8[i], v8[j]);
	fails += u8_check (v8[i], v8[j]);
      }
  return fails;
}
