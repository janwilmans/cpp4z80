/* Target definitions for the Z80.
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

#ifndef GCC_Z80_H
#define GCC_Z80_H

/* ------------------------------------------------------------------------- */
/* Run-time target specification                                             */
/* ------------------------------------------------------------------------- */

#define TARGET_CPU_CPP_BUILTINS()                                              \
  do                                                                           \
    {                                                                          \
      builtin_define ("__Z80__");                                              \
      builtin_define ("__z80__");                                              \
      builtin_assert ("cpu=z80");                                              \
      builtin_assert ("machine=z80");                                          \
    }                                                                          \
  while (0)

/* ------------------------------------------------------------------------- */
/* Driver                                                                    */
/* ------------------------------------------------------------------------- */

/* Link the default start-up code from libgcc (crt0.S).  There are no
   crtbegin/crtend files: constructors use .init_array.  */
#undef STARTFILE_SPEC
#define STARTFILE_SPEC "crt0.o%s"
#undef ENDFILE_SPEC
#define ENDFILE_SPEC ""

/* ------------------------------------------------------------------------- */
/* Storage layout                                                            */
/* ------------------------------------------------------------------------- */

#define BITS_BIG_ENDIAN 0
#define BYTES_BIG_ENDIAN 0
#define WORDS_BIG_ENDIAN 0

/* The Z80 is an 8-bit machine: a "word" in the GCC sense is one byte.
   16-bit values live in register pairs (two consecutive hard registers),
   just like on the AVR port.

   libgcc2.c derives its "word" and "double word" types from
   UNITS_PER_WORD; like AVR, pretend 32-bit words there so that it provides
   the 32/64-bit (long / long long) routines.  The 8/16/32-bit multiply,
   divide and shift helpers come from config/z80/lib1funcs.S.  */
#ifdef IN_LIBGCC2
#define UNITS_PER_WORD 4
#else
#define UNITS_PER_WORD 1
#endif

#define POINTER_SIZE 16
#define Pmode HImode
#define POINTERS_EXTEND_UNSIGNED 1

#define PARM_BOUNDARY 8
#define STACK_BOUNDARY 8
#define FUNCTION_BOUNDARY 8
#define BIGGEST_ALIGNMENT 8
#define STRICT_ALIGNMENT 0

#define SHORT_TYPE_SIZE 16
#define INT_TYPE_SIZE 16
#define LONG_TYPE_SIZE 32
#define LONG_LONG_TYPE_SIZE 64

#define DEFAULT_SIGNED_CHAR 0

#undef SIZE_TYPE
#define SIZE_TYPE "unsigned int"
#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "int"
#undef WCHAR_TYPE
#define WCHAR_TYPE "int"
#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE 16

/* ------------------------------------------------------------------------- */
/* Register numbers                                                          */
/* ------------------------------------------------------------------------- */

/* A 16-bit value occupies two consecutive hard registers and, because the
   target is little-endian, the LOW byte must have the LOWER register number
   (subreg byte 0 maps to the first register).  That is why the pairs are
   numbered C,B / E,D / L,H rather than in their "natural" B,C order.

   SImode values occupy four consecutive registers, so the pairs are laid out
   such that BC:DE and DE:HL are contiguous.  */

#define C_REG 0
#define B_REG 1
#define E_REG 2
#define D_REG 3
#define L_REG 4
#define H_REG 5
#define A_REG 6
#define IXL_REG 7
#define IXH_REG 8
#define IYL_REG 9
#define IYH_REG 10
#define SP_REG 11
/* Soft registers that are always eliminated (to IX).  */
#define FP_REG 12
#define AP_REG 13

#define FIRST_PSEUDO_REGISTER 14

#define REGISTER_NAMES                                                         \
  {"c", "b", "e", "d", "l", "h", "a", "ixl", "ixh", "iyl", "iyh", "sp",       \
   "?fp", "?ap"}

/* IX is the hard frame pointer.  IY is reserved as a scratch register that
   is only used inside single insns: the prologue, stack adjustments and
   (ix+d) accesses whose displacement does not fit in a byte.  SP and the
   soft registers are fixed as usual.  */
#define FIXED_REGISTERS                                                        \
  {                                                                            \
    0, 0, /* C B */                                                            \
    0, 0, /* E D */                                                            \
    0, 0, /* L H */                                                            \
    0,    /* A */                                                              \
    1, 1, /* IXL IXH */                                                        \
    1, 1, /* IYL IYH */                                                        \
    1,    /* SP */                                                             \
    1, 1  /* ?fp ?ap */                                                        \
  }

/* All allocatable registers are clobbered by calls.  IX is preserved.  */
#define CALL_USED_REGISTERS                                                    \
  {                                                                            \
    1, 1, /* C B */                                                            \
    1, 1, /* E D */                                                            \
    1, 1, /* L H */                                                            \
    1,    /* A */                                                              \
    0, 0, /* IXL IXH */                                                        \
    1, 1, /* IYL IYH */                                                        \
    1,    /* SP */                                                             \
    1, 1  /* ?fp ?ap */                                                        \
  }

/* Prefer the registers that are not used for argument passing first, and
   keep A last: it is needed by almost every 8-bit ALU instruction.  */
#define REG_ALLOC_ORDER                                                        \
  {C_REG, B_REG, E_REG, D_REG, L_REG, H_REG, A_REG, IXL_REG, IXH_REG,         \
   IYL_REG, IYH_REG, SP_REG, FP_REG, AP_REG}

/* ------------------------------------------------------------------------- */
/* Register classes                                                          */
/* ------------------------------------------------------------------------- */

enum reg_class
{
  NO_REGS,
  A_REGS,	/* a */
  BC_REGS,	/* b: BC pair */
  DE_REGS,	/* d: DE pair */
  HL_REGS,	/* h: HL pair */
  LOW_REGS,	/* l: C, E, L - the low halves of the pairs */
  BCDE_REGS,	/* w: BC, DE */
  PAIR_REGS,	/* BC, DE, HL */
  IX_REGS,	/* x */
  IY_REGS,	/* y */
  SP_REGS,	/* k */
  GENERAL_REGS, /* r: A, B, C, D, E, H, L */
  BASE_REGS,	/* BC, DE, HL, IX and the soft frame/argument pointers */
  ALL_REGS,
  LIM_REG_CLASSES
};

#define N_REG_CLASSES ((int) LIM_REG_CLASSES)

#define REG_CLASS_NAMES                                                        \
  {"NO_REGS",  "A_REGS",  "BC_REGS", "DE_REGS",      "HL_REGS",               \
   "LOW_REGS", "BCDE_REGS", "PAIR_REGS", "IX_REGS", "IY_REGS", "SP_REGS",    \
   "GENERAL_REGS", "BASE_REGS", "ALL_REGS"}

/* Bit N corresponds to hard register N.  */
#define REG_CLASS_CONTENTS                                                     \
  {                                                                            \
    {0x0000}, /* NO_REGS:      */                                              \
    {0x0040}, /* A_REGS:       a */                                            \
    {0x0003}, /* BC_REGS:      c b */                                          \
    {0x000c}, /* DE_REGS:      e d */                                          \
    {0x0030}, /* HL_REGS:      l h */                                          \
    {0x0015}, /* LOW_REGS:     c e l */                                        \
    {0x000f}, /* BCDE_REGS:    c b e d */                                      \
    {0x003f}, /* PAIR_REGS:    c b e d l h */                                  \
    {0x0180}, /* IX_REGS:      ixl ixh */                                      \
    {0x0600}, /* IY_REGS:      iyl iyh */                                      \
    {0x0800}, /* SP_REGS:      sp */                                           \
    {0x007f}, /* GENERAL_REGS: c b e d l h a */                                \
    {0x31bf}, /* BASE_REGS:    bc de hl ix ?fp ?ap */                         \
    {0x3fff}  /* ALL_REGS */                                                   \
  }

#define REGNO_REG_CLASS(REGNO) z80_regno_reg_class (REGNO)

#define BASE_REG_CLASS BASE_REGS
#define INDEX_REG_CLASS NO_REGS

#define REGNO_OK_FOR_BASE_P(REGNO) z80_regno_ok_for_base_p ((REGNO), true)
#define REGNO_OK_FOR_INDEX_P(REGNO) 0

/* ------------------------------------------------------------------------- */
/* Stack layout and frame                                                    */
/* ------------------------------------------------------------------------- */

/* Frame layout (IX is the hard frame pointer):

	(ix+4+n)   incoming stack arguments
	(ix+2)     return address
	(ix+0)     saved IX
	(ix-1)...  local variables and spill slots
	           outgoing pushed arguments (sp)  */

#define STACK_GROWS_DOWNWARD 1
/* Locals are allocated downwards from IX, so that every elimination
   offset is a constant.  (Growing upwards from a soft frame pointer below
   IX would make the FP->IX offset depend on the frame size, which changes
   while LRA allocates spill slots.)  */
#define FRAME_GROWS_DOWNWARD 1
#define FIRST_PARM_OFFSET(FNDECL) 0

#define STACK_POINTER_REGNUM SP_REG
#define FRAME_POINTER_REGNUM FP_REG
#define HARD_FRAME_POINTER_REGNUM IXL_REG
#define ARG_POINTER_REGNUM AP_REG
#define STATIC_CHAIN_REGNUM C_REG

/* (sp+d) is not an addressing mode, so everything must be eliminated into
   the hard frame pointer IX.  */
#define ELIMINABLE_REGS                                                        \
  {                                                                            \
    {ARG_POINTER_REGNUM, HARD_FRAME_POINTER_REGNUM},                           \
    {FRAME_POINTER_REGNUM, HARD_FRAME_POINTER_REGNUM},                         \
  }

#define INITIAL_ELIMINATION_OFFSET(FROM, TO, OFFSET)                           \
  ((OFFSET) = z80_initial_elimination_offset ((FROM), (TO)))

/* On entry the return address is the word at (sp); it was pushed by the
   call, so the CFA (the value of SP before the call) is SP + 2.  Needed
   for DWARF call frame information (-g).  */
#define INCOMING_RETURN_ADDR_RTX                                               \
  gen_rtx_MEM (HImode, gen_rtx_REG (HImode, SP_REG))
#define INCOMING_FRAME_SP_OFFSET 2

/* Only 16-bit pushes exist; a byte argument still takes up two bytes.  */
#define PUSH_ROUNDING(BYTES) z80_push_rounding (BYTES)

/* ------------------------------------------------------------------------- */
/* Function arguments                                                        */
/* ------------------------------------------------------------------------- */

struct z80_args
{
  /* Number of argument registers (pairs) used so far.  */
  int nregs;
  /* Variadic functions get all their arguments on the stack.  */
  bool stdarg;
};

typedef struct z80_args CUMULATIVE_ARGS;

#define INIT_CUMULATIVE_ARGS(CUM, FNTYPE, LIBNAME, FNDECL, N_NAMED_ARGS)       \
  z80_init_cumulative_args (&(CUM), (FNTYPE), (LIBNAME), (FNDECL))

#define FUNCTION_ARG_REGNO_P(REGNO)                                            \
  ((REGNO) == L_REG || (REGNO) == H_REG || (REGNO) == E_REG                    \
   || (REGNO) == D_REG)

#define FUNCTION_PROFILER(FILE, LABELNO)                                       \
  do                                                                           \
    {                                                                          \
    }                                                                          \
  while (0)

#define TRAMPOLINE_SIZE 4
#define TRAMPOLINE_ALIGNMENT 8

/* ------------------------------------------------------------------------- */
/* Addressing and misc                                                       */
/* ------------------------------------------------------------------------- */

#define MAX_REGS_PER_ADDRESS 1
#define FUNCTION_MODE QImode
#define CASE_VECTOR_MODE HImode
#define MOVE_MAX 2
#define SLOW_BYTE_ACCESS 0
#define NO_FUNCTION_CSE 1
#define STORE_FLAG_VALUE 1

#define BRANCH_COST(speed_p, predictable_p) 1

/* ------------------------------------------------------------------------- */
/* Assembler output                                                          */
/* ------------------------------------------------------------------------- */

#define ASM_COMMENT_START ";"
#define ASM_APP_ON ""
#define ASM_APP_OFF ""

#define TEXT_SECTION_ASM_OP "\t.text"
#define DATA_SECTION_ASM_OP "\t.data"
#undef BSS_SECTION_ASM_OP
#define BSS_SECTION_ASM_OP "\t.section\t.bss"

#undef GLOBAL_ASM_OP
#define GLOBAL_ASM_OP "\t.globl\t"

/* The z80 assembler does not accept \" in strings, which elfos.h uses.  */
#undef ASM_OUTPUT_LIMITED_STRING
#undef ASM_OUTPUT_ASCII
#define ASM_OUTPUT_ASCII(FILE, STR, LENGTH)                                    \
  z80_output_ascii ((FILE), (STR), (LENGTH))

/* Jump tables for switch statements hold absolute 16-bit addresses.  They
   need no alignment (elfos.h would align them to 4 bytes).  */
#undef ASM_OUTPUT_BEFORE_CASE_LABEL
#define ASM_OUTPUT_BEFORE_CASE_LABEL(FILE, PREFIX, NUM, TABLE)                 \
  do                                                                           \
    {                                                                          \
    }                                                                          \
  while (0)

#define ASM_OUTPUT_ADDR_VEC_ELT(FILE, VALUE)                                   \
  fprintf ((FILE), "\t.word\t.L%d\n", (VALUE))
#define ASM_OUTPUT_ADDR_DIFF_ELT(FILE, BODY, VALUE, REL)                       \
  fprintf ((FILE), "\t.word\t.L%d-.L%d\n", (VALUE), (REL))

#define FINAL_PRESCAN_INSN(INSN, OPVEC, NOPERANDS)                             \
  z80_final_prescan_insn ((INSN), (OPVEC), (NOPERANDS))

#define ASM_OUTPUT_ALIGN(FILE, LOG)                                            \
  do                                                                           \
    {                                                                          \
      if ((LOG) != 0)                                                          \
	fprintf ((FILE), "\t.p2align\t%d\n", (LOG));                            \
    }                                                                          \
  while (0)

#endif /* GCC_Z80_H */
