#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "target.h"
#include "tm.h"

#include "rtl.h"
#include "tree.h"
#include "stringpool.h"
#include "expr.h"
#include "regs.h"
#include "function.h"
#include "calls.h"
#include "builtins.h"
#include "attribs.h"
#include "output.h"
#include "hooks.h"

static rtx
z80_function_arg (cumulative_args_t cum_v, const function_arg_info &arg)
{
  CUMULATIVE_ARGS *cum = get_cumulative_args (cum_v);
  (void) cum;
  (void) arg;

  return NULL_RTX;
}

static void
z80_function_arg_advance (cumulative_args_t cum_v, const function_arg_info &arg)
{
  CUMULATIVE_ARGS *cum = get_cumulative_args (cum_v);
  (void) cum;
  (void) arg;
}

static bool
z80_functions_b_resolvable_from_a (tree a, tree b, tree c)
{
  (void) a;
  (void) b;
  (void) c;

  return false;
}

#undef TARGET_FUNCTION_ARG
#define TARGET_FUNCTION_ARG z80_function_arg

#undef TARGET_FUNCTION_ARG_ADVANCE
#define TARGET_FUNCTION_ARG_ADVANCE z80_function_arg_advance

#undef TARGET_OPTION_FUNCTIONS_B_RESOLVABLE_FROM_A
#define TARGET_OPTION_FUNCTIONS_B_RESOLVABLE_FROM_A                            \
  z80_functions_b_resolvable_from_a

/*
 * This file should be included last.
 */
#include "target-def.h"

struct gcc_target targetm = TARGET_INITIALIZER;
