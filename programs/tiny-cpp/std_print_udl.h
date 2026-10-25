// std_print ("text"_s): the macro-free variant.
//
// "text"_s is a C++20 string-literal operator template: the characters are
// a template argument, so the '$'-terminated copy is made at compile time
// and lives in the template parameter object.  std_print itself is an
// ordinary inline function.

#ifndef STD_PRINT_UDL_H
#define STD_PRINT_UDL_H

#include "tiny.h"

// A string literal of N chars (including its NUL), with '$' in place of
// the NUL: the same N bytes as the literal itself.
template <unsigned N>
struct fixed_string
{
  char data[N];

  consteval fixed_string (const char (&s)[N]) : data{}
  {
    for (unsigned i = 0; i + 1 < N; i++)
      data[i] = s[i];
    data[N - 1] = '$';
  }
};

struct dollar_literal
{
  const char *data;
};

template <fixed_string S>
consteval dollar_literal operator""_s ()
{
  return { S.data };
}

inline void std_print (dollar_literal s)
{
  puts_ (s.data);
}

#endif
