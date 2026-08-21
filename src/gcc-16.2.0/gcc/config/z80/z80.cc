#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "target.h"
#include "tm.h"
#include "rtl.h"
#include "tree.h"
#include "expr.h"
#include "regs.h"
#include "function.h"

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

#undef TARGET_FUNCTION_ARG
#define TARGET_FUNCTION_ARG z80_function_arg

#undef TARGET_FUNCTION_ARG_ADVANCE
#define TARGET_FUNCTION_ARG_ADVANCE z80_function_arg_advance

#include "target-def.h"

struct gcc_target targetm = TARGET_INITIALIZER;
