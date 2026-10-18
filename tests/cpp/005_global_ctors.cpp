// Global objects with constructors (run by crt0 through .init_array).
// expect: 0
struct Logger
{
  int value;
  Logger (int v) : value (v * 2) {}
};

Logger first (21);
Logger second (first.value + 8);

int main ()
{
  if (first.value != 42) return 1;
  if (second.value != 100) return 2;
  return 0;
}
