// _Complex float arithmetic and arguments (8-byte values on the stack).
// expect: 0
__attribute__ ((noipa)) _Complex float cmul (_Complex float a, _Complex float b)
{
  return a * b;
}

__attribute__ ((noipa)) float re (_Complex float z) { return __real__ z; }
__attribute__ ((noipa)) float im (_Complex float z) { return __imag__ z; }

int main (void)
{
  _Complex float a = 1.0f + 2.0f * __extension__ 1.0if;
  _Complex float b = 3.0f - 1.0f * __extension__ 1.0if;
  _Complex float p = cmul (a, b);	/* (1+2i)(3-i) = 5 + 5i */
  if (re (p) != 5.0f || im (p) != 5.0f)
    return 1;
  return 0;
}
