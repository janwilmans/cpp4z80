// A small template vector with new/delete, copy and move semantics,
// destructors (RAII) and range-based for.  There is no libstdc++ (yet), so
// operator new/delete are defined here on top of malloc/free.
// expect: 0
// output: squares: 0 1 4 9 16 25 36 49 64 81
// output: live objects after scope: 0
#include <stdio.h>
#include <stdlib.h>

void *operator new (size_t n) { return malloc (n); }
void *operator new[] (size_t n) { return malloc (n); }
void operator delete (void *p) noexcept { free (p); }
void operator delete[] (void *p) noexcept { free (p); }
void operator delete (void *p, size_t) noexcept { free (p); }
void operator delete[] (void *p, size_t) noexcept { free (p); }

template <typename T> class Vector
{
public:
  Vector () = default;
  Vector (const Vector &other) { for (const T &x : other) push_back (x); }
  Vector (Vector &&other) noexcept
    : data_ (other.data_), size_ (other.size_), cap_ (other.cap_)
  {
    other.data_ = nullptr;
    other.size_ = other.cap_ = 0;
  }
  ~Vector () { delete[] data_; }

  void push_back (const T &x)
  {
    if (size_ == cap_)
      grow ();
    data_[size_++] = x;
  }
  unsigned size () const { return size_; }
  T &operator[] (unsigned i) { return data_[i]; }
  T *begin () { return data_; }
  T *end () { return data_ + size_; }
  const T *begin () const { return data_; }
  const T *end () const { return data_ + size_; }

private:
  void grow ()
  {
    unsigned cap = cap_ ? cap_ * 2 : 4;
    T *data = new T[cap];
    for (unsigned i = 0; i < size_; i++)
      data[i] = data_[i];
    delete[] data_;
    data_ = data;
    cap_ = cap;
  }

  T *data_ = nullptr;
  unsigned size_ = 0, cap_ = 0;
};

static int live;

struct Tracked
{
  int v;
  Tracked (int x = 0) : v (x) { live++; }
  Tracked (const Tracked &o) : v (o.v) { live++; }
  Tracked &operator= (const Tracked &o) { v = o.v; return *this; }
  ~Tracked () { live--; }
};

static Vector<int> squares (int n)
{
  Vector<int> v;
  for (int i = 0; i < n; i++)
    v.push_back (i * i);
  return v;
}

int main ()
{
  Vector<int> s = squares (10);
  printf ("squares:");
  for (int x : s)
    printf (" %d", x);
  printf ("\n");

  {
    Vector<Tracked> t;
    for (int i = 0; i < 20; i++)
      t.push_back (Tracked (i));
    Vector<Tracked> copy (t);
    long sum = 0;
    for (const Tracked &x : copy)
      sum += x.v;
    if (sum != 190)
      return 1;
  }
  printf ("live objects after scope: %d\n", live);
  return live;
}
