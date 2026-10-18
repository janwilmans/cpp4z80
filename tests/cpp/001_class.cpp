// A simple class with constructor, member functions and an object on the
// stack.
// expect: 42
class Counter
{
public:
  Counter (int start) : value (start) {}
  void add (int n) { value += n; }
  int get () const { return value; }

private:
  int value;
};

__attribute__ ((noipa)) int run (int start)
{
  Counter c (start);
  for (int i = 0; i < 4; i++)
    c.add (i);
  return c.get ();
}

int main ()
{
  return run (36);
}
