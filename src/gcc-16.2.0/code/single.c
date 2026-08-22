
// test for compiling a single function, but not main() because that would
// require initialization/startup code as well and this test should be the
// absolute minimum.
unsigned char
example (void)
{
  return 42;
}
