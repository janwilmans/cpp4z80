// Virtual functions: dispatch through a vtable (indirect calls).
// expect: 0
struct Shape
{
  virtual int area () const = 0;
  virtual int sides () const { return 0; }
};

struct Rect : Shape
{
  int w, h;
  Rect (int w_, int h_) : w (w_), h (h_) {}
  int area () const override { return w * h; }
  int sides () const override { return 4; }
};

struct Square : Rect
{
  Square (int s) : Rect (s, s) {}
};

struct Triangle : Shape
{
  int b, h;
  Triangle (int b_, int h_) : b (b_), h (h_) {}
  int area () const override { return b * h / 2; }
  int sides () const override { return 3; }
};

__attribute__ ((noipa)) int total_area (const Shape *const *shapes, int n)
{
  int t = 0;
  for (int i = 0; i < n; i++)
    t += shapes[i]->area () + shapes[i]->sides ();
  return t;
}

int main ()
{
  Rect r (3, 4);
  Square s (5);
  Triangle t (6, 4);
  const Shape *shapes[] = { &r, &s, &t };
  return total_area (shapes, 3) - (12 + 4 + 25 + 4 + 12 + 3);
}
