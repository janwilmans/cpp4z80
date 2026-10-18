// Recursive backtracking sudoku solver.
// expect: 0
// output: 534678912
// output: 672195348
// output: 198342567
// output: 859761423
// output: 426853791
// output: 713924856
// output: 961537284
// output: 287419635
// output: 345286179
#include <stdio.h>

static unsigned char grid[9][9] = {
  { 5, 3, 0, 0, 7, 0, 0, 0, 0 },
  { 6, 0, 0, 1, 9, 5, 0, 0, 0 },
  { 0, 9, 8, 0, 0, 0, 0, 6, 0 },
  { 8, 0, 0, 0, 6, 0, 0, 0, 3 },
  { 4, 0, 0, 8, 0, 3, 0, 0, 1 },
  { 7, 0, 0, 0, 2, 0, 0, 0, 6 },
  { 0, 6, 0, 0, 0, 0, 2, 8, 0 },
  { 0, 0, 0, 4, 1, 9, 0, 0, 5 },
  { 0, 0, 0, 0, 8, 0, 0, 7, 9 },
};

static int allowed (int r, int c, unsigned char v)
{
  for (int i = 0; i < 9; i++)
    if (grid[r][i] == v || grid[i][c] == v)
      return 0;
  int br = r / 3 * 3, bc = c / 3 * 3;
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      if (grid[br + i][bc + j] == v)
	return 0;
  return 1;
}

static int solve (int pos)
{
  if (pos == 81)
    return 1;
  int r = pos / 9, c = pos % 9;
  if (grid[r][c])
    return solve (pos + 1);
  for (unsigned char v = 1; v <= 9; v++)
    if (allowed (r, c, v))
      {
	grid[r][c] = v;
	if (solve (pos + 1))
	  return 1;
      }
  grid[r][c] = 0;
  return 0;
}

int main (void)
{
  if (!solve (0))
    return 1;
  for (int r = 0; r < 9; r++)
    {
      for (int c = 0; c < 9; c++)
	putchar ('0' + grid[r][c]);
      putchar ('\n');
    }
  return 0;
}
