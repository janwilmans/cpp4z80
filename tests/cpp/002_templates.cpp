// Function and class templates instantiated for 8 and 16-bit types.
// expect: 0
template <typename T> __attribute__ ((noipa)) T max_of (const T *p, int n)
{
  T m = p[0];
  for (int i = 1; i < n; i++)
    if (p[i] > m)
      m = p[i];
  return m;
}

template <typename T, int N> struct Stack
{
  T data[N];
  int top = 0;
  void push (T v) { data[top++] = v; }
  T pop () { return data[--top]; }
  bool empty () const { return top == 0; }
};

int main ()
{
  const int words[] = { 3, -7, 1200, 5 };
  const unsigned char bytes[] = { 3, 250, 17 };
  if (max_of (words, 4) != 1200) return 1;
  if (max_of (bytes, 3) != 250) return 2;

  Stack<int, 8> s;
  for (int i = 1; i <= 5; i++)
    s.push (i * 100);
  int sum = 0;
  while (!s.empty ())
    sum += s.pop ();
  if (sum != 1500) return 3;
  return 0;
}
