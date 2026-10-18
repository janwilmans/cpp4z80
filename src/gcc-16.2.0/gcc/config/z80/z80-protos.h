/* Prototypes for the Z80 backend.
   Copyright (C) 2026 Free Software Foundation, Inc.
   Contributed by Jan Wilmans.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.
*/

#ifndef GCC_Z80_PROTOS_H
#define GCC_Z80_PROTOS_H

extern enum reg_class z80_regno_reg_class (int regno);
extern bool z80_regno_ok_for_base_p (int regno, bool strict);
extern HOST_WIDE_INT z80_initial_elimination_offset (int from, int to);
extern poly_int64 z80_push_rounding (poly_int64 bytes);

extern void z80_expand_prologue (void);
extern void z80_expand_epilogue (void);

#ifdef RTX_CODE
extern void z80_init_cumulative_args (CUMULATIVE_ARGS *cum, tree fntype,
				      rtx libname, tree fndecl);
extern bool z80_move_ok_p (rtx dst, rtx src);
extern const char *z80_output_movqi (rtx *operands);
extern const char *z80_output_movhi (rtx *operands);
extern const char *z80_output_addhi3 (rtx *operands);
extern const char *z80_output_shift (enum rtx_code code, rtx *operands);
extern const char *z80_output_cbranch (rtx *operands, bool hi);
extern const char *z80_output_prologue (rtx *operands);
extern const char *z80_output_sp_adjust (HOST_WIDE_INT delta);
extern bool z80_q_memory_p (rtx op);
extern bool z80_abs_memory_p (rtx op);
#endif

#endif /* GCC_Z80_PROTOS_H */
