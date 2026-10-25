// std_print ("text") prints a string literal through BDOS function 9.
//
// That call wants the text to end in '$'.  The '$' copy of the literal is
// made at compile time (constexpr), so the program contains only the one
// string, with its '$' where the terminating NUL would have been, and no
// code that builds or measures it.

#ifndef STD_PRINT_H
#define STD_PRINT_H

#include "tiny.h"

// A string literal of N chars (including its NUL), with '$' in place of
// the NUL: the same N bytes as the literal itself.
template <unsigned N>
struct dollar_string
{
  char data[N];

  constexpr dollar_string (const char (&s)[N]) : data{}
  {
    for (unsigned i = 0; i + 1 < N; i++)
      data[i] = s[i];
    data[N - 1] = '$';
  }
};

// A macro, because only a literal written at the call site can be turned
// into a constexpr object; a function parameter never is one.
#define std_print(literal)						\
  do									\
    {									\
      static constexpr dollar_string dollar_literal{literal};		\
      puts_ (dollar_literal.data);					\
    }									\
  while (0)

#endif
