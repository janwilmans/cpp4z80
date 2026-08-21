/* GCC backend definitions for the Z80 processor.

	 Copyright (C) 2026 Free Software Foundation, Inc.

	 This file is part of GCC.

	 GCC is free software; you can redistribute it and/or modify it
	 under the terms of the GNU General Public License as published
	 by the Free Software Foundation; either version 3, or (at your
	 option) any later version.

	 GCC is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 GNU General Public License for more details.

	 You should have received a copy of the GNU General Public License
	 along with GCC.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef GCC_Z80_H
#define GCC_Z80_H

#define SLOW_BYTE_ACCESS 0

#define ASM_OUTPUT_ALIGN(FILE, POWER) fprintf ((FILE), "\t.align %d\n", (POWER))
#define GLOBAL_ASM_OP "\t.globl "

#define ASM_APP_ON ""
#define ASM_APP_OFF ""

#define FUNCTION_PROFILER(FILE, LABELNO)                                       \
  do                                                                           \
    {                                                                          \
    }                                                                          \
  while (0)

typedef unsigned int CUMULATIVE_ARGS;

#define INIT_CUMULATIVE_ARGS(CUM, FNTYPE, LIBNAME, FNDECL, N_NAMED_ARGS)       \
  ((CUM) = 0)

#define TRAMPOLINE_SIZE 4
#define TRAMPOLINE_ALIGNMENT 16

#define TARGET_CPU_CPP_BUILTINS()                                              \
  do                                                                           \
    {                                                                          \
      builtin_define ("__Z80__");                                              \
      builtin_assert ("cpu=z80");                                              \
      builtin_assert ("machine=z80");                                          \
    }                                                                          \
  while (0)

/*
 * The Z80 is an 8-bit processor.
 *
 * Registers:
 *
 *   0  A
 *   1  F
 *   2  B
 *   3  C
 *   4  D
 *   5  E
 *   6  H
 *   7  L
 *
 *   8  IXH
 *   9  IXL
 *  10  IYH
 *  11  IYL
 *
 *  12  SP
 *  13  PC
 *
 *  14  HL' / alternate register bank
 *  15  BC'
 *  16  DE'
 *
 *  17  I
 *  18  R
 *
 * This is intentionally a first approximation.  The register model
 * can be refined as the instruction patterns are implemented.
 */

#define FIRST_PSEUDO_REGISTER 19

#define A_REG 0
#define F_REG 1
#define B_REG 2
#define C_REG 3
#define D_REG 4
#define E_REG 5
#define H_REG 6
#define L_REG 7
#define IXH_REG 8
#define IXL_REG 9
#define IYH_REG 10
#define IYL_REG 11
#define SP_REG 12
#define PC_REG 13
#define HL_ALT_REG 14
#define BC_ALT_REG 15
#define DE_ALT_REG 16
#define I_REG 17
#define R_REG 18

#define STACK_POINTER_REGNUM SP_REG

#define REGISTER_NAMES                                                         \
  {"a",	  "f",	 "b",  "c",  "d",   "e",   "h",	  "l", "ixh", "ixl",           \
   "iyh", "iyl", "sp", "pc", "hl'", "bc'", "de'", "i", "r"}

#define ADDITIONAL_REGISTER_NAMES                                              \
  {                                                                            \
    {"af", A_REG}, {"bc", B_REG}, {"de", D_REG}, {"hl", H_REG},                \
      {"ix", IXH_REG},                                                         \
    {                                                                          \
      "iy", IYH_REG                                                            \
    }                                                                          \
  }

#define FIXED_REGISTERS                                                        \
  {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1}

#define CALL_USED_REGISTERS                                                    \
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}

#define REG_ALLOC_ORDER                                                        \
  {A_REG, C_REG,   B_REG,   E_REG,   D_REG,  L_REG,                            \
   H_REG, IXL_REG, IXH_REG, IYL_REG, IYH_REG}

#define BITS_BIG_ENDIAN 0
#define BYTES_BIG_ENDIAN 0
#define WORDS_BIG_ENDIAN 0

#define UNITS_PER_WORD 1

#define SHORT_TYPE_SIZE 16
#define INT_TYPE_SIZE 16
#define LONG_TYPE_SIZE 32
#define LONG_LONG_TYPE_SIZE 64

#define POINTER_SIZE 16
#define Pmode HImode
#define CASE_VECTOR_MODE HImode

#define FUNCTION_BOUNDARY 8
#define BIGGEST_ALIGNMENT 8
#define STACK_BOUNDARY 8
#define PARM_BOUNDARY 8

#define STACK_GROWS_DOWNWARD 1
#define FRAME_GROWS_DOWNWARD 1

#define FIRST_PARM_OFFSET(FNDECL) 0

#define STRICT_ALIGNMENT 1

#define MOVE_MAX 2

#define MAX_REGS_PER_ADDRESS 1

#define FUNCTION_MODE HImode

#define DEFAULT_SIGNED_CHAR 0

#define POINTERS_EXTEND_UNSIGNED 1

#define FRAME_POINTER_REGNUM 13

#define ARG_POINTER_REGNUM 13

#define STATIC_CHAIN_REGNUM 2

#define FUNCTION_VALUE_REGNO_P(REGNO) ((REGNO) == A_REG)

#define LIBCALL_VALUE(MODE) gen_rtx_REG ((MODE), A_REG)

#define FUNCTION_ARG_REGNO_P(REGNO) 0

/*
 * Register classes.
 */

enum reg_class
{
  NO_REGS,
  A_REGS,
  BC_REGS,
  DE_REGS,
  HL_REGS,
  IX_REGS,
  IY_REGS,
  GENERAL_REGS,
  SP_REGS,
  ALL_REGS,
  LIM_REG_CLASSES
};

#define N_REG_CLASSES ((int) LIM_REG_CLASSES)

#define REG_CLASS_NAMES                                                        \
  {"NO_REGS", "A_REGS",	 "BC_REGS",	 "DE_REGS", "HL_REGS",                 \
   "IX_REGS", "IY_REGS", "GENERAL_REGS", "SP_REGS", "ALL_REGS"}

#define REG_CLASS_CONTENTS                                                     \
  {{0x00000},  {0x00001},  {0x0000c},  {0x000030}, {0x0000c0},                 \
   {0x000300}, {0x000c00}, {0x000fff}, {0x001000}, {0x007fff}}

#define REGNO_REG_CLASS(REGNO)                                                 \
  ((REGNO) < FIRST_PSEUDO_REGISTER ? GENERAL_REGS : NO_REGS)

#define REGNO_OK_FOR_BASE_P(REGNO) ((REGNO) < FIRST_PSEUDO_REGISTER)

#define REGNO_OK_FOR_INDEX_P(REGNO) REGNO_OK_FOR_BASE_P (REGNO)

#define BASE_REG_CLASS GENERAL_REGS
#define INDEX_REG_CLASS GENERAL_REGS

#define CLASS_MAX_NREGS(CLASS, MODE)                                           \
  ((GET_MODE_SIZE (MODE) + UNITS_PER_WORD - 1) / UNITS_PER_WORD)

#define TARGET_SMALL_REGISTER_CLASSES_FOR_MODE_P hook_bool_mode_true

#define ELIMINABLE_REGS                                                        \
  {                                                                            \
    {ARG_POINTER_REGNUM, STACK_POINTER_REGNUM},                                \
      {ARG_POINTER_REGNUM, FRAME_POINTER_REGNUM},                              \
    {                                                                          \
      FRAME_POINTER_REGNUM, STACK_POINTER_REGNUM                               \
    }                                                                          \
  }

#define INITIAL_ELIMINATION_OFFSET(FROM, TO, OFFSET) ((OFFSET) = 0)

#endif /* GCC_Z80_H */
