// Mandelbrot set as ASCII art: fixed-point arithmetic in long, a small
// class, printf output.
// expect: 0
// output: .....:::------------=====++#@@@====---:::
// output: ...::-----------======+**#@@@@@#+++==----
// output: ..:----------===+++*#@@@@@@@@@@@@@@@@+=--
// output: .:-----==+*@#######@@@@@@@@@@@@@@@@@@%==-
// output: .=====+++##@@@@@@@@@@@@@@@@@@@@@@@@@@*+==
// output: @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@#++==
// output: .-====+++*#@@@@@@@@@@@@@@@@@@@@@@@@@@*+==
// output: .:-----==+*@#######@@@@@@@@@@@@@@@@@@%==-
// output: ..:----------===+++*#@@@@@@@@@@@@@@@@+=--
// output: ...::-----------======+**#@@@@@#+++==----
// output: .....:::------------=====++#@@@====---:::
#include <stdio.h>

class Fixed
{
public:
  static constexpr int frac = 10;
  static constexpr long one = 1L << frac;

  static long mul (long a, long b) { return (a * b) >> frac; }
};

static int iterations (long cr, long ci)
{
  long zr = 0, zi = 0;
  int it = 0;
  for (; it < 9; it++)
    {
      long zr2 = Fixed::mul (zr, zr), zi2 = Fixed::mul (zi, zi);
      if (zr2 + zi2 > 4 * Fixed::one)
	break;
      zi = Fixed::mul (2 * zr, zi) + ci;
      zr = zr2 - zi2 + cr;
    }
  return it;
}

int main ()
{
  static const char chars[] = " .:-=+*#%@";
  for (int py = -10; py <= 10; py += 2)
    {
      char line[48];
      int len = 0;
      for (int px = -30; px <= 10; px++)
	line[len++] = chars[iterations (px * Fixed::one / 15,
					py * Fixed::one / 10)];
      while (len > 0 && line[len - 1] == ' ')
	len--;
      line[len] = 0;
      printf ("%s\n", line);
    }
  return 0;
}
