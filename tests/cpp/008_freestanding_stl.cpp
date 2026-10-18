// The freestanding C++ standard library: <array>, <algorithm>, <numeric>,
// <optional>, <variant>, <tuple>, <span>, <string_view>, <bit>, <ranges>,
// <utility> and <limits>.
// expect: 0
// output: sorted: 1 2 3 5 8 13 21
// output: sum=53 max=21 popcount(0xf0f0)=8
// output: variant holds int 7, then "z80"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <limits>
#include <numeric>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <stdio.h>

static std::optional<int> find_index (std::span<const int> s, int value)
{
  for (std::size_t i = 0; i < s.size (); i++)
    if (s[i] == value)
      return static_cast<int> (i);
  return std::nullopt;
}

static std::tuple<int, int> min_max (std::span<const int> s)
{
  auto [lo, hi] = std::minmax_element (s.begin (), s.end ());
  return { *lo, *hi };
}

struct Printer
{
  void operator() (int i) const { printf ("int %d", i); }
  void operator() (std::string_view s) const
  {
    printf ("\"%.*s\"", static_cast<int> (s.size ()), s.data ());
  }
};

int main ()
{
  std::array<int, 7> a = { 13, 2, 8, 1, 21, 5, 3 };
  std::sort (a.begin (), a.end ());
  printf ("sorted:");
  for (int x : a)
    printf (" %d", x);
  printf ("\n");

  int sum = std::accumulate (a.begin (), a.end (), 0);
  auto [lo, hi] = min_max (a);
  printf ("sum=%d max=%d popcount(0xf0f0)=%d\n", sum, hi,
	  std::popcount (0xf0f0u));
  if (lo != 1)
    return 1;

  if (find_index (a, 8) != 4 || find_index (a, 4).has_value ())
    return 2;

  std::variant<int, std::string_view> v = 7;
  printf ("variant holds ");
  std::visit (Printer {}, v);
  v = std::string_view ("z80");
  printf (", then ");
  std::visit (Printer {}, v);
  printf ("\n");

  auto evens = a | std::views::filter ([] (int x) { return x % 2 == 0; });
  int n = 0;
  for (int x : evens)
    n += x;
  if (n != 10)
    return 3;

  static_assert (std::numeric_limits<int>::max () == 32767);
  static_assert (std::numeric_limits<long>::digits == 31);
  static_assert (sizeof (void *) == 2 && CHAR_BIT == 8);
  return 0;
}
