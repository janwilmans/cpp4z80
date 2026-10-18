/* Subroutines for the Z80 backend.
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

#define IN_TARGET_CODE 1

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "target.h"
#include "rtl.h"
#include "tree.h"
#include "stringpool.h"
#include "attribs.h"
#include "df.h"
#include "memmodel.h"
#include "tm_p.h"
#include "regs.h"
#include "emit-rtl.h"
#include "insn-config.h"
#include "recog.h"
#include "diagnostic-core.h"
#include "output.h"
#include "stor-layout.h"
#include "calls.h"
#include "explow.h"
#include "expr.h"
#include "builtins.h"
#include "function.h"
#include "insn-attr.h"

/* This file should be included last.  */
#include "target-def.h"

/* ------------------------------------------------------------------------- */
/* Register handling                                                         */
/* ------------------------------------------------------------------------- */

/* Implement REGNO_REG_CLASS: the smallest class containing REGNO.  */

enum reg_class
z80_regno_reg_class (int regno)
{
  switch (regno)
    {
    case A_REG:
      return A_REGS;
    case C_REG:
    case B_REG:
      return BC_REGS;
    case E_REG:
    case D_REG:
      return DE_REGS;
    case L_REG:
    case H_REG:
      return HL_REGS;
    case IXL_REG:
    case IXH_REG:
      return IX_REGS;
    case IYL_REG:
    case IYH_REG:
      return IY_REGS;
    case SP_REG:
      return SP_REGS;
    case FP_REG:
    case AP_REG:
      return BASE_REGS;
    default:
      return NO_REGS;
    }
}

/* Return true if REGNO can be used as a base register: HL, BC, DE, IX, or
   one of the soft frame/argument pointers (which are eliminated into IX).

   Only HL (and IX) can really address memory for every instruction;
   (bc) and (de) are output by temporarily redirecting the access through
   HL (see z80_redirect_begin).  Offering three pointer registers instead
   of one is what keeps LRA from running out of registers.  */

bool
z80_regno_ok_for_base_p (int regno, bool strict)
{
  if (regno >= FIRST_PSEUDO_REGISTER)
    {
      if (!strict)
	return true;
      if (!reg_renumber || reg_renumber[regno] < 0)
	return false;
      regno = reg_renumber[regno];
    }

  return (regno == L_REG || regno == C_REG || regno == E_REG
	  || regno == IXL_REG || regno == FP_REG || regno == AP_REG);
}

/* Registers that hold a complete 16-bit value in ONE hard register.  */

static bool
z80_single_16bit_reg_p (unsigned int regno)
{
  return regno == SP_REG || regno == FP_REG || regno == AP_REG;
}

/* Implement TARGET_HARD_REGNO_NREGS.  */

static unsigned int
z80_hard_regno_nregs (unsigned int regno, machine_mode mode)
{
  if (z80_single_16bit_reg_p (regno))
    return 1;
  return CEIL (GET_MODE_SIZE (mode), UNITS_PER_WORD);
}

/* Implement TARGET_HARD_REGNO_MODE_OK.

   The 8-bit registers are A, B, C, D, E, H, L.  16-bit values live in
   the register pairs BC, DE, HL, IX, IY (named by their low register) or
   in SP and the soft frame/argument pointers.  32-bit values live in
   BC:DE or DE:HL.  */

static bool
z80_hard_regno_mode_ok (unsigned int regno, machine_mode mode)
{
  switch (GET_MODE_SIZE (mode))
    {
    case 1:
      return regno <= A_REG;

    case 2:
      return (regno == C_REG || regno == E_REG || regno == L_REG
	      || regno == IXL_REG || regno == IYL_REG
	      || z80_single_16bit_reg_p (regno));

    case 4:
      return regno == C_REG || regno == E_REG;

    default:
      return false;
    }
}

/* Implement TARGET_CLASS_LIKELY_SPILLED_P.  All the small classes are:
   there are only three allocatable register pairs, and HL is needed for
   almost every memory access.  This stops combine from propagating hard
   registers (e.g. a return value in HL) into insns that need HL for
   something else, which LRA could not resolve.  */

static bool
z80_class_likely_spilled_p (reg_class_t rclass)
{
  return rclass != GENERAL_REGS && rclass != BASE_REGS && rclass != ALL_REGS;
}

/* Implement TARGET_MODES_TIEABLE_P.  */

static bool
z80_modes_tieable_p (machine_mode mode1, machine_mode mode2)
{
  return mode1 == mode2;
}

/* ------------------------------------------------------------------------- */
/* Addressing                                                                */
/* ------------------------------------------------------------------------- */

/* Return the size in bytes of an access in MODE (at least 1).  */

static HOST_WIDE_INT
z80_access_size (machine_mode mode)
{
  if (mode == VOIDmode || mode == BLKmode)
    return 1;
  HOST_WIDE_INT size = GET_MODE_SIZE (mode);
  return size > 0 ? size : 1;
}

/* Return true if an access of SIZE bytes at displacement OFFSET can be
   encoded directly as (ix+d), i.e. every byte is within -128..127.  */

static bool
z80_short_displacement_p (HOST_WIDE_INT offset, HOST_WIDE_INT size)
{
  return offset >= -128 && offset + size - 1 <= 127;
}

/* Return true if (BASE + OFFSET) is a valid indexed address.  The base must
   be IX (or one of the registers that are eliminated into IX).

   Any 16-bit displacement is accepted: displacements outside -128..127 are
   handled when the insn is output, by pointing IY at the location first
   (see z80_final_prescan_insn).  Rejecting them would force the address
   into HL, the only other pointer register, which is often not free.  */

static bool
z80_indexed_address_p (rtx base, HOST_WIDE_INT offset, bool strict)
{
  if (!REG_P (base) || !IN_RANGE (offset, -32768, 32767))
    return false;

  unsigned int regno = REGNO (base);
  if (regno >= FIRST_PSEUDO_REGISTER)
    /* The virtual frame/argument registers are replaced by the soft
       frame/argument pointers, which in turn are eliminated into IX.
       Any other pseudo cannot become IX (IX is fixed), so it can only
       ever be used as (hl), which has no displacement.  */
    return (!strict && regno >= FIRST_VIRTUAL_REGISTER
	    && regno <= LAST_VIRTUAL_REGISTER);

  return regno == AP_REG || regno == FP_REG || regno == IXL_REG;
}

/* Implement TARGET_LEGITIMATE_ADDRESS_P.

   Valid addresses are:

     (hl)          (reg HL)
     (ix+d)        (plus (reg IX) (const_int d)); d outside -128..127 is
		   output through IY
     (nn)          a constant address

   Push addresses (pre_dec sp) only appear inside the explicit push
   patterns and are deliberately not accepted here, so that the move
   patterns never match a push.  */

static bool
z80_legitimate_address_p (machine_mode, rtx x, bool strict,
			  code_helper = ERROR_MARK)
{
  switch (GET_CODE (x))
    {
    case REG:
      return z80_regno_ok_for_base_p (REGNO (x), strict);

    case PLUS:
      if (CONST_INT_P (XEXP (x, 1)))
	return z80_indexed_address_p (XEXP (x, 0), INTVAL (XEXP (x, 1)),
				      strict);
      return false;

    case CONST_INT:
      return IN_RANGE (INTVAL (x), 0, 0xffff);

    case SYMBOL_REF:
    case LABEL_REF:
    case CONST:
      return true;

    default:
      return false;
    }
}

/* Return true if OP is a memory reference with a register or (ix+d)
   address: (hl), (bc), (de), (ix+d).  Used by the "Q" constraint.  */

bool
z80_q_memory_p (rtx op)
{
  if (!MEM_P (op))
    return false;

  rtx addr = XEXP (op, 0);

  if (REG_P (addr))
    {
      /* Before register allocation a pseudo address will be allocated to
	 the base register class.  */
      if (REGNO (addr) >= FIRST_PSEUDO_REGISTER)
	return !reload_completed;
      return z80_regno_ok_for_base_p (REGNO (addr), reload_completed);
    }

  if (GET_CODE (addr) == PLUS && CONST_INT_P (XEXP (addr, 1)))
    return z80_indexed_address_p (XEXP (addr, 0), INTVAL (XEXP (addr, 1)),
				  false);

  return false;
}

/* Return true if OP is a memory reference with a constant address.
   Used by the "S" constraint.  */

bool
z80_abs_memory_p (rtx op)
{
  return MEM_P (op) && CONSTANT_P (XEXP (op, 0));
}

/* ------------------------------------------------------------------------- */
/* Operand printing                                                          */
/* ------------------------------------------------------------------------- */

/* Return the assembler name of the 16-bit register that starts at REGNO.  */

static const char *
z80_pair_name (unsigned int regno)
{
  switch (regno)
    {
    case C_REG:
      return "bc";
    case E_REG:
      return "de";
    case L_REG:
      return "hl";
    case IXL_REG:
      return "ix";
    case IYL_REG:
      return "iy";
    case SP_REG:
      return "sp";
    default:
      gcc_unreachable ();
    }
}

/* Return the name of the push/pop-able pair that contains the 8-bit
   register REGNO.  */

static const char *
z80_containing_pair_name (unsigned int regno)
{
  switch (regno)
    {
    case C_REG:
    case B_REG:
      return "bc";
    case E_REG:
    case D_REG:
      return "de";
    case L_REG:
    case H_REG:
      return "hl";
    case A_REG:
      return "af";
    default:
      gcc_unreachable ();
    }
}

/* Memory addressed by BC or DE is accessed through HL:

     (de):  ex de,hl  /  ...(hl)...  /  ex de,hl
     (bc):  push hl / ld h,b / ld l,c  /  ...(hl)...  /  pop hl

   None of these instructions change the flags.  While a (de) access is
   redirected, D/E and H/L have swapped places, so register operands are
   printed with swapped names.  The (bc) form must not be used by an insn
   that writes H or L.  Z80_REDIRECT_BASE is the base register being
   redirected (C_REG or E_REG), or -1.  */

static int z80_redirect_base = -1;

/* Return C_REG or E_REG if MEM is addressed by BC or DE, otherwise -1.  */

static int
z80_mem_redirect_base (rtx mem)
{
  if (!MEM_P (mem) || !REG_P (XEXP (mem, 0)))
    return -1;
  unsigned int regno = REGNO (XEXP (mem, 0));
  return regno == C_REG || regno == E_REG ? (int) regno : -1;
}

/* Return the register that holds what REGNO holds while the current
   redirection is active.  */

static unsigned int
z80_redirected_regno (unsigned int regno)
{
  if (z80_redirect_base != E_REG)
    return regno;
  switch (regno)
    {
    case E_REG:
      return L_REG;
    case D_REG:
      return H_REG;
    case L_REG:
      return E_REG;
    case H_REG:
      return D_REG;
    default:
      return regno;
    }
}

static void
z80_redirect_begin (int base)
{
  gcc_assert (z80_redirect_base < 0);
  if (base == C_REG)
    output_asm_insn ("push\thl\n\tld\th,b\n\tld\tl,c", NULL);
  else
    output_asm_insn ("ex\tde,hl", NULL);
  z80_redirect_base = base;
}

static void
z80_redirect_end (void)
{
  if (z80_redirect_base == C_REG)
    output_asm_insn ("pop\thl", NULL);
  else if (z80_redirect_base == E_REG)
    output_asm_insn ("ex\tde,hl", NULL);
  z80_redirect_base = -1;
}

/* The out-of-range (ix+d) address of the insn being output, and the
   displacement that IY has been set to point at (relative to IX).  Set by
   z80_final_prescan_insn.  */

static rtx z80_far_addr;
static HOST_WIDE_INT z80_far_base;

/* Implement FINAL_PRESCAN_INSN.

   If one of the operands of INSN is an (ix+d) memory reference whose
   displacement does not fit in a signed byte, emit code that points IY at
   IX+d first; the operand is then printed as (iy+0) / (iy+1).  IY is a
   fixed scratch register that is never live between insns, and the
   sequence preserves all other registers.  */

void
z80_final_prescan_insn (rtx_insn *insn, rtx *operands, int noperands)
{
  z80_far_addr = NULL_RTX;
  z80_redirect_base = -1;

  /* (bc)/(de) operands of arithmetic insns: these only write A, so the
     access can be redirected around the whole insn.  Moves and jumps do
     this themselves.  */
  int code = INSN_CODE (insn);
  if (!JUMP_P (insn) && code != CODE_FOR_movqi_insn
      && code != CODE_FOR_movhi_insn)
    for (int i = 0; i < noperands; i++)
      {
	int base = z80_mem_redirect_base (operands[i]);
	if (base >= 0)
	  {
	    z80_redirect_begin (base);
	    break;
	  }
      }

  for (int i = 0; i < noperands; i++)
    {
      rtx op = operands[i];
      if (!op || !MEM_P (op))
	continue;

      rtx addr = XEXP (op, 0);
      if (GET_CODE (addr) != PLUS
	  || !REG_P (XEXP (addr, 0))
	  || REGNO (XEXP (addr, 0)) != IXL_REG
	  || !CONST_INT_P (XEXP (addr, 1)))
	continue;

      HOST_WIDE_INT disp = INTVAL (XEXP (addr, 1));
      if (z80_short_displacement_p (disp, z80_access_size (GET_MODE (op))))
	continue;

      if (z80_far_addr)
	{
	  /* All far references of one insn must be reachable from the
	     same IY value.  */
	  gcc_assert (rtx_equal_p (addr, z80_far_addr));
	  continue;
	}

      z80_far_addr = addr;
      z80_far_base = disp;
      fprintf (asm_out_file, "\tpush\tix\n\tpop\tiy\n\tpush\tbc\n"
	       "\tld\tbc,%d\n\tadd\tiy,bc\n\tpop\tbc\n", (int) disp);
    }
}

/* Implement TARGET_ASM_FINAL_POSTSCAN_INSN: finish a redirection started
   by z80_final_prescan_insn.  */

static void
z80_final_postscan_insn (FILE *, rtx_insn *, rtx *, int)
{
  if (z80_redirect_base >= 0)
    z80_redirect_end ();
}

/* Print the memory address ADDR, displaced by OFFSET bytes.  */

static void
z80_print_address (FILE *file, rtx addr, HOST_WIDE_INT offset)
{
  rtx base = addr;
  HOST_WIDE_INT disp = offset;

  if (GET_CODE (addr) == PLUS && CONST_INT_P (XEXP (addr, 1)))
    {
      base = XEXP (addr, 0);
      disp += INTVAL (XEXP (addr, 1));
    }

  if (REG_P (base))
    {
      unsigned int regno = REGNO (base);
      if (disp == 0 && (regno == L_REG || (int) regno == z80_redirect_base))
	fputs ("(hl)", file);
      else if (disp == 0 && (regno == C_REG || regno == E_REG))
	/* Only "ld a,(bc)" style instructions get here.  */
	fprintf (file, "(%s)", z80_pair_name (regno));
      else if (regno == IXL_REG && z80_far_addr
	       && rtx_equal_p (addr, z80_far_addr))
	fprintf (file, "(iy%+d)", (int) (disp - z80_far_base));
      else if (regno == IXL_REG && IN_RANGE (disp, -128, 127))
	fprintf (file, "(ix%+d)", (int) disp);
      else
	output_operand_lossage ("invalid Z80 address");
      return;
    }

  if (CONSTANT_P (addr))
    {
      fputc ('(', file);
      output_addr_const (file, addr);
      if (offset)
	fprintf (file, "+%d", (int) offset);
      fputc (')', file);
      return;
    }

  output_operand_lossage ("invalid Z80 address");
}

/* Implement TARGET_PRINT_OPERAND.

   Modifiers:
     L   low byte of a 16-bit register / constant / memory operand
     H   high byte of a 16-bit register / constant / memory operand
     P   the push/pop-able pair containing an 8-bit register
     c   a constant without decoration (for call/jump targets)  */

static void
z80_print_operand (FILE *file, rtx x, int code)
{
  switch (GET_CODE (x))
    {
    case REG:
      {
	unsigned int regno = z80_redirected_regno (REGNO (x));
	if (code == 'P')
	  fputs (z80_containing_pair_name (regno), file);
	else if (code == 'L')
	  fputs (reg_names[regno], file);
	else if (code == 'H')
	  fputs (reg_names[regno + 1], file);
	else if (GET_MODE_SIZE (GET_MODE (x)) == 2)
	  fputs (z80_pair_name (regno), file);
	else
	  fputs (reg_names[regno], file);
	return;
      }

    case MEM:
      z80_print_address (file, XEXP (x, 0), code == 'H' ? 1 : 0);
      return;

    case CONST_INT:
      {
	HOST_WIDE_INT val = INTVAL (x);
	if (code == 'L')
	  val &= 0xff;
	else if (code == 'H')
	  val = (val >> 8) & 0xff;
	else
	  /* Print as a signed 16-bit value: GCC sign-extends constants to
	     their mode, so an 8-bit -2 must stay -2 (not 65534).  */
	  val = trunc_int_for_mode (val, HImode);
	fprintf (file, HOST_WIDE_INT_PRINT_DEC, val);
	return;
      }

    case SYMBOL_REF:
    case LABEL_REF:
    case CONST:
      if (code == 'L')
	{
	  fputs ("lo(", file);
	  output_addr_const (file, x);
	  fputc (')', file);
	}
      else if (code == 'H')
	{
	  fputs ("hi(", file);
	  output_addr_const (file, x);
	  fputc (')', file);
	}
      else
	output_addr_const (file, x);
      return;

    default:
      output_operand_lossage ("unsupported Z80 operand");
      return;
    }
}

/* Implement ASM_OUTPUT_ASCII.  Output LENGTH bytes of STR as .ascii
   directives, escaping quotes, backslashes and non-printable characters
   in octal: the z80 assembler does not understand \".  */

void
z80_output_ascii (FILE *file, const char *str, size_t length)
{
  size_t column = 0;
  for (size_t i = 0; i < length; i++)
    {
      unsigned char c = str[i];
      if (column == 0)
	fputs ("\t.ascii\t\"", file);
      if (c == '"' || c == '\\' || !ISPRINT (c))
	column += fprintf (file, "\\%03o", c);
      else
	{
	  fputc (c, file);
	  column++;
	}
      if (column >= 64 || i == length - 1)
	{
	  fputs ("\"\n", file);
	  column = 0;
	}
    }
}

/* Implement TARGET_PRINT_OPERAND_ADDRESS.  */

static void
z80_print_operand_address (FILE *file, machine_mode, rtx addr)
{
  z80_print_address (file, addr, 0);
}

/* ------------------------------------------------------------------------- */
/* Move output                                                               */
/* ------------------------------------------------------------------------- */

/* Return true if a move from SRC to DST is directly supported (the insn
   condition for the move patterns).  */

bool
z80_move_ok_p (rtx dst, rtx src)
{
  return !(MEM_P (dst) && MEM_P (src));
}

/* Return the 16-bit piece at byte OFFSET of X, an operand of a MODE move.
   For memory whose address is not offsettable (e.g. (hl)), *BASE caches a
   register holding the address so that it is only computed once.  */

static rtx
z80_wide_move_part (rtx x, machine_mode mode, int offset, rtx *base)
{
  if (!MEM_P (x))
    return simplify_gen_subreg (HImode, x, mode, offset);

  rtx addr = XEXP (x, 0);
  if (offset == 0 || offsettable_address_p (true, mode, addr))
    return adjust_address (x, HImode, offset);

  /* (mem (reg)) or similar: address the other pieces relative to a copy of
     the base address.  */
  gcc_assert (can_create_pseudo_p ());
  if (!*base)
    *base = force_reg (Pmode, addr);
  rtx part_addr = force_reg (Pmode, plus_constant (Pmode, *base, offset));
  return adjust_automodify_address (x, HImode, part_addr, offset);
}

/* Expand a move of a 32- or 64-bit value as a sequence of 16-bit moves.  */

void
z80_expand_wide_move (rtx dst, rtx src)
{
  machine_mode mode = GET_MODE (dst);
  int size = GET_MODE_SIZE (mode);

  /* A push (e.g. one half of a complex value): push 16-bit pieces, the
     highest first.  */
  if (push_operand (dst, mode))
    {
      for (int offset = size - 2; offset >= 0; offset -= 2)
	{
	  rtx half = simplify_gen_subreg (HImode, src, mode, offset);
	  emit_insn (gen_pushhi1 (force_reg (HImode, half)));
	}
      return;
    }

  if (can_create_pseudo_p ())
    {
      if (MEM_P (dst) && !register_operand (src, mode) && !CONSTANT_P (src))
	src = force_reg (mode, src);

      /* Do not move between memory and a hard register (e.g. a return value
	 in DE:HL) directly: the memory address may need HL itself.  Going
	 through a pseudo keeps the hard register's live range short.  */
      if (MEM_P (dst) && REG_P (src) && HARD_REGISTER_P (src))
	src = copy_to_mode_reg (mode, src);
      else if (MEM_P (src) && REG_P (dst) && HARD_REGISTER_P (dst))
	{
	  rtx tmp = gen_reg_rtx (mode);
	  z80_expand_wide_move (tmp, src);
	  src = tmp;
	}
    }

  /* Moving between overlapping register groups (e.g. BC:DE to DE:HL) must
     start with the piece that is overwritten last.  */
  bool backwards = false;
  if (REG_P (dst) && REG_P (src) && REGNO (dst) > REGNO (src)
      && REGNO (dst) < REGNO (src) + size)
    backwards = true;

  rtx dst_base = NULL_RTX, src_base = NULL_RTX;
  for (int i = 0; i < size; i += 2)
    {
      int offset = backwards ? size - 2 - i : i;
      rtx d = z80_wide_move_part (dst, mode, offset, &dst_base);
      rtx s = z80_wide_move_part (src, mode, offset, &src_base);
      emit_move_insn (d, s);
    }
}

/* Output an 8-bit move.  Every alternative is a single "ld", except for
   (bc)/(de) accesses, which are redirected through HL - or through A when
   the other operand is H or L, which the (bc) redirection would destroy.  */

const char *
z80_output_movqi (rtx *operands)
{
  rtx dst = operands[0];
  rtx src = operands[1];

  if (REG_P (dst) && REG_P (src) && REGNO (dst) == REGNO (src))
    return "";

  rtx mem = MEM_P (dst) ? dst : MEM_P (src) ? src : NULL_RTX;
  int base = mem ? z80_mem_redirect_base (mem) : -1;
  if (base < 0)
    return "ld\t%0,%1";

  rtx other = mem == dst ? src : dst;
  if (REG_P (other) && REGNO (other) == A_REG)
    /* ld a,(bc) / ld (de),a etc.  */
    return "ld\t%0,%1";

  if (base == C_REG && REG_P (other)
      && (REGNO (other) == H_REG || REGNO (other) == L_REG))
    return "push\taf\n\tld\ta,%1\n\tld\t%0,a\n\tpop\taf";

  z80_redirect_begin (base);
  output_asm_insn ("ld\t%0,%1", operands);
  z80_redirect_end ();
  return "";
}

static bool
z80_ix_memory_p (rtx mem)
{
  rtx addr = XEXP (mem, 0);
  if (GET_CODE (addr) == PLUS)
    addr = XEXP (addr, 0);
  return REG_P (addr) && REGNO (addr) == IXL_REG;
}

/* Output a 16-bit load of operand 0 from (hl).  DST is the register that
   operand 0 is in at this point (it differs from REGNO (operands[0]) while
   a (de) access is redirected).  */

static void
z80_output_load_hl (rtx *operands, unsigned int dst)
{
  if (dst == L_REG)
    output_asm_insn ("push\tde\n\tld\te,(hl)\n\tinc\thl\n\tld\td,(hl)\n\t"
		     "ex\tde,hl\n\tpop\tde", operands);
  else
    output_asm_insn ("ld\t%L0,(hl)\n\tinc\thl\n\tld\t%H0,(hl)\n\tdec\thl",
		     operands);
}

/* Output a 16-bit store of operand 1 to (hl).  SRC is the register that
   holds operand 1 at this point, or -1 for a constant.  */

static void
z80_output_store_hl (rtx *operands, int src)
{
  if (src == L_REG)
    output_asm_insn ("push\tde\n\tld\te,l\n\tld\td,h\n\tld\t(hl),e\n\t"
		     "inc\thl\n\tld\t(hl),d\n\tdec\thl\n\tpop\tde", operands);
  else
    output_asm_insn ("ld\t(hl),%L1\n\tinc\thl\n\tld\t(hl),%H1\n\tdec\thl",
		     operands);
}

/* Output a 16-bit move.  The Z80 has no "ld rr,rr'" and only HL (and the
   ED-prefixed forms of BC/DE) can use absolute addresses, so most cases
   are composed from 8-bit moves.  No sequence clobbers any register that
   is not the destination: push/pop is used for temporaries instead.  */

const char *
z80_output_movhi (rtx *operands)
{
  rtx dst = operands[0];
  rtx src = operands[1];

  if (REG_P (dst))
    {
      unsigned int d = REGNO (dst);

      if (REG_P (src))
	{
	  unsigned int s = REGNO (src);

	  if (d == s)
	    return "";
	  if (d == SP_REG)
	    {
	      if (s == L_REG || s == IXL_REG || s == IYL_REG)
		return "ld\tsp,%1";
	      return "push\t%1\n\tpop\tiy\n\tld\tsp,iy";
	    }
	  if (s == SP_REG)
	    {
	      if (d == L_REG)
		return "ld\thl,0\n\tadd\thl,sp";
	      if (d == IXL_REG || d == IYL_REG)
		return "ld\t%0,0\n\tadd\t%0,sp";
	      return "ld\tiy,0\n\tadd\tiy,sp\n\tpush\tiy\n\tpop\t%0";
	    }
	  if (d >= IXL_REG || s >= IXL_REG)
	    /* Moves involving IX or IY.  */
	    return "push\t%1\n\tpop\t%0";
	  return "ld\t%H0,%H1\n\tld\t%L0,%L1";
	}

      if (CONSTANT_P (src))
	return "ld\t%0,%1";

      gcc_assert (MEM_P (src));
      rtx addr = XEXP (src, 0);

      if (CONSTANT_P (addr))
	return "ld\t%0,%1";

      if (z80_ix_memory_p (src))
	return "ld\t%L0,%L1\n\tld\t%H0,%H1";

      switch (REGNO (addr))
	{
	case L_REG:
	  z80_output_load_hl (operands, d);
	  break;
	case E_REG:
	  z80_redirect_begin (E_REG);
	  z80_output_load_hl (operands, z80_redirected_regno (d));
	  z80_redirect_end ();
	  break;
	case C_REG:
	  if (d == L_REG)
	    {
	      /* The destination is HL itself: no need to preserve it.  */
	      output_asm_insn ("ld\th,b\n\tld\tl,c", operands);
	      z80_output_load_hl (operands, d);
	    }
	  else
	    {
	      z80_redirect_begin (C_REG);
	      z80_output_load_hl (operands, d);
	      z80_redirect_end ();
	    }
	  break;
	default:
	  gcc_unreachable ();
	}
      return "";
    }

  gcc_assert (MEM_P (dst));
  rtx addr = XEXP (dst, 0);

  if (CONSTANT_P (addr))
    {
      gcc_assert (REG_P (src));
      return "ld\t%0,%1";
    }

  if (z80_ix_memory_p (dst))
    {
      if (CONST_INT_P (src) || REG_P (src))
	return "ld\t%L0,%L1\n\tld\t%H0,%H1";
      gcc_unreachable ();
    }

  int s = REG_P (src) ? (int) REGNO (src) : -1;
  switch (REGNO (addr))
    {
    case L_REG:
      z80_output_store_hl (operands, s);
      break;
    case E_REG:
      z80_redirect_begin (E_REG);
      z80_output_store_hl (operands, s < 0 ? s : z80_redirected_regno (s));
      z80_redirect_end ();
      break;
    case C_REG:
      if (s == L_REG)
	/* Copy the value to DE first: the redirection needs HL.  */
	output_asm_insn ("push\tde\n\tld\te,l\n\tld\td,h\n\tpush\thl\n\t"
			 "ld\th,b\n\tld\tl,c\n\tld\t(hl),e\n\tinc\thl\n\t"
			 "ld\t(hl),d\n\tpop\thl\n\tpop\tde", operands);
      else
	{
	  z80_redirect_begin (C_REG);
	  z80_output_store_hl (operands, s);
	  z80_redirect_end ();
	}
      break;
    default:
      gcc_unreachable ();
    }
  return "";
}

/* ------------------------------------------------------------------------- */
/* 16-bit addition                                                           */
/* ------------------------------------------------------------------------- */

/* Output a sequence that adds the constant VAL to the 16-bit register
   REG (which already holds the first addend).  */

static void
z80_output_add_const (rtx reg, HOST_WIDE_INT val)
{
  unsigned int regno = REGNO (reg);
  rtx ops[2] = { reg, GEN_INT (trunc_int_for_mode (val, HImode)) };

  val = trunc_int_for_mode (val, HImode);
  if (val == 0)
    return;

  if (val >= -4 && val <= 4)
    {
      for (; val > 0; val--)
	output_asm_insn ("inc\t%0", ops);
      for (; val < 0; val++)
	output_asm_insn ("dec\t%0", ops);
      return;
    }

  switch (regno)
    {
    case L_REG:
      output_asm_insn ("push\tde\n\tld\tde,%1\n\tadd\thl,de\n\tpop\tde", ops);
      break;
    case E_REG:
      output_asm_insn ("ex\tde,hl\n\tpush\tbc\n\tld\tbc,%1\n\tadd\thl,bc\n\t"
		       "pop\tbc\n\tex\tde,hl", ops);
      break;
    case C_REG:
      output_asm_insn ("push\thl\n\tld\thl,%1\n\tadd\thl,bc\n\tld\tb,h\n\t"
		       "ld\tc,l\n\tpop\thl", ops);
      break;
    case SP_REG:
      output_asm_insn ("ld\tiy,%1\n\tadd\tiy,sp\n\tld\tsp,iy", ops);
      break;
    default:
      gcc_unreachable ();
    }
}

/* Output (set (reg D) (plus (reg D) (reg S))) for the pairs BC/DE/HL.
   Only HL can be the destination of "add rr,rr'"; the other cases go
   through HL using ex de,hl or push/pop.  */

static void
z80_output_add_reg (rtx dst, rtx src)
{
  unsigned int d = REGNO (dst);
  unsigned int s = REGNO (src);
  rtx ops[2] = { dst, src };

  switch (d)
    {
    case L_REG:
      output_asm_insn ("add\thl,%1", ops);
      break;

    case E_REG:
      /* Swap DE and HL; then HL holds the destination value, and the
	 source (if it was DE or HL) has swapped as well.  */
      if (s == C_REG)
	output_asm_insn ("ex\tde,hl\n\tadd\thl,bc\n\tex\tde,hl", ops);
      else if (s == L_REG)
	output_asm_insn ("ex\tde,hl\n\tadd\thl,de\n\tex\tde,hl", ops);
      else
	output_asm_insn ("ex\tde,hl\n\tadd\thl,hl\n\tex\tde,hl", ops);
      break;

    case C_REG:
      if (s == L_REG)
	output_asm_insn ("push\thl\n\tadd\thl,bc\n\tld\tb,h\n\tld\tc,l\n\t"
			 "pop\thl", ops);
      else if (s == E_REG)
	output_asm_insn ("push\thl\n\tld\th,b\n\tld\tl,c\n\tadd\thl,de\n\t"
			 "ld\tb,h\n\tld\tc,l\n\tpop\thl", ops);
      else
	output_asm_insn ("push\thl\n\tld\th,b\n\tld\tl,c\n\tadd\thl,hl\n\t"
			 "ld\tb,h\n\tld\tc,l\n\tpop\thl", ops);
      break;

    default:
      gcc_unreachable ();
    }
}

/* Output the addhi3 pattern.  Operand 0 is the destination, operand 1 the
   first addend (tied to operand 0, or IX), operand 2 a register or a
   constant.  */

const char *
z80_output_addhi3 (rtx *operands)
{
  rtx dst = operands[0];
  rtx op1 = operands[1];
  rtx op2 = operands[2];

  if (REGNO (op1) != REGNO (dst))
    {
      /* dst = IX + const (the address of a stack slot).  */
      gcc_assert (REGNO (op1) == IXL_REG && CONST_INT_P (op2));
      output_asm_insn ("push\tix\n\tpop\t%0", operands);
    }

  if (CONST_INT_P (op2))
    z80_output_add_const (dst, INTVAL (op2));
  else if (REGNO (dst) == L_REG || REGNO (dst) == IXL_REG)
    output_asm_insn ("add\t%0,%2", operands);
  else
    z80_output_add_reg (dst, op2);
  return "";
}

/* Output (set (reg D) (minus (reg D) OP2)) for the pairs BC/DE/HL.  Only
   "sbc hl,rr" exists, so the other pairs go through HL.  */

const char *
z80_output_subhi3 (rtx *operands)
{
  rtx dst = operands[0];
  rtx op2 = operands[2];

  if (REGNO (op2) == REGNO (dst))
    return "ld\t%0,0";

  switch (REGNO (dst))
    {
    case L_REG:
      return "or\ta\n\tsbc\thl,%2";

    case E_REG:
      /* After the swap DE holds what was in HL and vice versa.  */
      if (REGNO (op2) == L_REG)
	return "ex\tde,hl\n\tor\ta\n\tsbc\thl,de\n\tex\tde,hl";
      return "ex\tde,hl\n\tor\ta\n\tsbc\thl,%2\n\tex\tde,hl";

    case C_REG:
      if (REGNO (op2) == L_REG)
	return ("push\tde\n\tld\td,h\n\tld\te,l\n\tpush\thl\n\t"
		"ld\th,b\n\tld\tl,c\n\tor\ta\n\tsbc\thl,de\n\t"
		"ld\tb,h\n\tld\tc,l\n\tpop\thl\n\tpop\tde");
      return ("push\thl\n\tld\th,b\n\tld\tl,c\n\tor\ta\n\t"
	      "sbc\thl,%2\n\tld\tb,h\n\tld\tc,l\n\tpop\thl");

    default:
      gcc_unreachable ();
    }
}

/* ------------------------------------------------------------------------- */
/* Shifts                                                                    */
/* ------------------------------------------------------------------------- */

/* Output a single-bit shift of operand 0 in direction CODE.  */

static void
z80_output_shift1 (enum rtx_code code, rtx *operands)
{
  rtx dst = operands[0];

  if (GET_MODE (dst) == QImode)
    {
      if (code == ASHIFT && REGNO (dst) == A_REG)
	output_asm_insn ("add\ta,a", operands);
      else if (code == ASHIFT)
	output_asm_insn ("sla\t%0", operands);
      else if (code == LSHIFTRT)
	output_asm_insn ("srl\t%0", operands);
      else
	output_asm_insn ("sra\t%0", operands);
      return;
    }

  if (code == ASHIFT && REGNO (dst) == L_REG)
    output_asm_insn ("add\thl,hl", operands);
  else if (code == ASHIFT)
    output_asm_insn ("sla\t%L0\n\trl\t%H0", operands);
  else if (code == LSHIFTRT)
    output_asm_insn ("srl\t%H0\n\trr\t%L0", operands);
  else
    output_asm_insn ("sra\t%H0\n\trr\t%L0", operands);
}

/* Output a shift of operand 0 (tied to operand 1) by operand 2 in
   direction CODE.  For a variable count operand 3 is a scratch QImode
   register used as loop counter.  */

const char *
z80_output_shift (enum rtx_code code, rtx *operands)
{
  rtx dst = operands[0];
  bool hi = GET_MODE (dst) == HImode;
  int bits = hi ? 16 : 8;

  if (!CONST_INT_P (operands[2]))
    {
      output_asm_insn ("ld\t%3,%2\n\tinc\t%3\n\tjr\t.Lt%=\n.Ll%=:",
		       operands);
      z80_output_shift1 (code, operands);
      output_asm_insn (".Lt%=:\n\tdec\t%3\n\tjr\tnz,.Ll%=", operands);
      return "";
    }

  HOST_WIDE_INT count = INTVAL (operands[2]) & (bits - 1);

  if (hi && count >= 8)
    {
      /* Move a whole byte first.  */
      if (code == ASHIFT)
	output_asm_insn ("ld\t%H0,%L0\n\tld\t%L0,0", operands);
      else if (code == LSHIFTRT)
	output_asm_insn ("ld\t%L0,%H0\n\tld\t%H0,0", operands);
      else
	output_asm_insn ("ld\t%L0,%H0\n\tsla\t%H0\n\tld\t%H0,0\n\t"
			 "jr\tnc,.Lp%=\n\tdec\t%H0\n.Lp%=:", operands);
      count -= 8;
      for (; count > 0; count--)
	{
	  if (code == ASHIFT)
	    output_asm_insn ("sla\t%H0", operands);
	  else if (code == LSHIFTRT)
	    output_asm_insn ("srl\t%L0", operands);
	  else
	    output_asm_insn ("sra\t%L0", operands);
	}
      return "";
    }

  for (; count > 0; count--)
    z80_output_shift1 (code, operands);
  return "";
}

/* ------------------------------------------------------------------------- */
/* Compare and branch                                                        */
/* ------------------------------------------------------------------------- */

/* Return the register that holds what was in REG after "ex de,hl".  */

static rtx
z80_swap_de_hl (rtx x)
{
  if (REG_P (x) && REGNO (x) == E_REG)
    return gen_rtx_REG (HImode, L_REG);
  if (REG_P (x) && REGNO (x) == L_REG)
    return gen_rtx_REG (HImode, E_REG);
  return x;
}

/* Output instructions that set the flags for HL - OP2 without changing
   any register.  OP2 is a pair or a constant.

   "add hl,rr" after "sbc hl,rr" restores HL without changing S, Z or
   P/V, and sets C exactly when the subtraction borrowed, so afterwards
   all flags still describe HL - OP2.  */

static void
z80_output_compare_hl (rtx op2)
{
  if (REG_P (op2) && REGNO (op2) == L_REG)
    /* Comparing HL with itself: equal.  "cp a" gives Z=1, C=0, S=0, V=0.  */
    output_asm_insn ("cp\ta", &op2);
  else if (REG_P (op2))
    output_asm_insn ("or\ta\n\tsbc\thl,%0\n\tadd\thl,%0", &op2);
  else
    output_asm_insn ("push\tde\n\tld\tde,%0\n\tor\ta\n\t"
		     "sbc\thl,de\n\tadd\thl,de\n\tpop\tde", &op2);
}

/* Output instructions that set the flags for OP1 - OP2 (16-bit) without
   changing any register.  OP1 is one of the pairs BC, DE, HL; OP2 a pair
   or a constant.  Only HL can be compared directly, so other pairs are
   swapped or copied into HL first ("ex", "push" and "pop" do not change
   the flags).  */

static void
z80_output_compare_hi (rtx op1, rtx op2)
{
  if (REG_P (op2) && REGNO (op2) == REGNO (op1))
    {
      output_asm_insn ("cp\ta", &op2);
      return;
    }

  switch (REGNO (op1))
    {
    case L_REG:
      z80_output_compare_hl (op2);
      break;

    case E_REG:
      output_asm_insn ("ex\tde,hl", &op1);
      z80_output_compare_hl (z80_swap_de_hl (op2));
      output_asm_insn ("ex\tde,hl", &op1);
      break;

    case C_REG:
      if (REG_P (op2) && REGNO (op2) == L_REG)
	output_asm_insn ("push\tde\n\tld\td,h\n\tld\te,l\n\tpush\thl\n\t"
			 "ld\th,b\n\tld\tl,c\n\tor\ta\n\tsbc\thl,de\n\t"
			 "pop\thl\n\tpop\tde", &op1);
      else
	{
	  output_asm_insn ("push\thl\n\tld\th,b\n\tld\tl,c", &op1);
	  z80_output_compare_hl (op2);
	  output_asm_insn ("pop\thl", &op1);
	}
      break;

    default:
      gcc_unreachable ();
    }
}

/* Output a compare-and-branch of operand 1 against zero, with the
   (already reversed if needed) condition CODE.

   8-bit: "or a" if the value is in A, otherwise "inc r / dec r", which
   leaves the register unchanged and sets Z and S from its value.
   16-bit: the sign is bit 7 of the high byte; equality with zero needs
   "ld a,h / or l", so the 16-bit zero-compare pattern clobbers A.

   Only Z and S are valid afterwards, which is all a comparison with zero
   needs; unsigned comparisons with zero are always/never true or reduce
   to (in)equality.  */

static const char *
z80_output_cbranch_zero (enum rtx_code code, rtx *operands)
{
  rtx op = operands[1];

  switch (code)
    {
    case LTU:
      return "";
    case GEU:
      return "jp\t%l3";
    case GTU:
      code = NE;
      break;
    case LEU:
      code = EQ;
      break;
    default:
      break;
    }

  if (GET_MODE (op) == HImode)
    {
      switch (code)
	{
	case LT:
	  return "bit\t7,%H1\n\tjp\tnz,%l3";
	case GE:
	  return "bit\t7,%H1\n\tjp\tz,%l3";
	case EQ:
	  return "ld\ta,%H1\n\tor\t%L1\n\tjp\tz,%l3";
	case NE:
	  return "ld\ta,%H1\n\tor\t%L1\n\tjp\tnz,%l3";
	case GT:
	  return ("bit\t7,%H1\n\tjr\tnz,.Ls%=\n\tld\ta,%H1\n\tor\t%L1\n\t"
		  "jp\tnz,%l3\n.Ls%=:");
	case LE:
	  return ("bit\t7,%H1\n\tjp\tnz,%l3\n\tld\ta,%H1\n\tor\t%L1\n\t"
		  "jp\tz,%l3");
	default:
	  gcc_unreachable ();
	}
    }

  if (REGNO (op) == A_REG)
    output_asm_insn ("or\ta", operands);
  else
    output_asm_insn ("inc\t%1\n\tdec\t%1", operands);

  switch (code)
    {
    case EQ:
      return "jp\tz,%l3";
    case NE:
      return "jp\tnz,%l3";
    case LT:
      return "jp\tm,%l3";
    case GE:
      return "jp\tp,%l3";
    case GT:
      return "jr\tz,.Ls%=\n\tjp\tp,%l3\n.Ls%=:";
    case LE:
      return "jp\tz,%l3\n\tjp\tm,%l3";
    default:
      gcc_unreachable ();
    }
}

/* Output a compare-and-branch.  Operand 0 is the comparison, operands 1 and
   2 the values compared and operand 3 the target label.  If REVERSED, the
   jump is taken when the comparison is false.

   After "cp" (8-bit) or "sbc hl,rr" (16-bit) the flags are:
     Z    equal
     C    unsigned less than (borrow)
     S^V  signed less than (sign, corrected by overflow)  */

const char *
z80_output_cbranch (rtx *operands, bool reversed)
{
  enum rtx_code code = GET_CODE (operands[0]);
  if (reversed)
    code = reverse_condition (code);

  if (operands[2] == const0_rtx)
    return z80_output_cbranch_zero (code, operands);

  if (GET_MODE (operands[1]) == QImode)
    {
      /* The flags survive the end of a (bc)/(de) redirection.  */
      int base = z80_mem_redirect_base (operands[2]);
      if (base >= 0)
	z80_redirect_begin (base);
      output_asm_insn ("cp\t%2", operands);
      if (base >= 0)
	z80_redirect_end ();
    }
  else
    z80_output_compare_hi (operands[1], operands[2]);

  switch (code)
    {
    case EQ:
      return "jp\tz,%l3";
    case NE:
      return "jp\tnz,%l3";
    case LTU:
      return "jp\tc,%l3";
    case GEU:
      return "jp\tnc,%l3";
    case GTU:
      return "jr\tz,.Ls%=\n\tjp\tnc,%l3\n.Ls%=:";
    case LEU:
      return "jp\tc,%l3\n\tjp\tz,%l3";
    case LT:
      return ("jp\tpe,.Lo%=\n\tjp\tm,%l3\n\tjr\t.Ls%=\n"
	      ".Lo%=:\n\tjp\tp,%l3\n.Ls%=:");
    case GE:
      return ("jp\tpe,.Lo%=\n\tjp\tp,%l3\n\tjr\t.Ls%=\n"
	      ".Lo%=:\n\tjp\tm,%l3\n.Ls%=:");
    case GT:
      return ("jr\tz,.Ls%=\n\tjp\tpe,.Lo%=\n\tjp\tp,%l3\n\tjr\t.Ls%=\n"
	      ".Lo%=:\n\tjp\tm,%l3\n.Ls%=:");
    case LE:
      return ("jp\tz,%l3\n\tjp\tpe,.Lo%=\n\tjp\tm,%l3\n\tjr\t.Ls%=\n"
	      ".Lo%=:\n\tjp\tp,%l3\n.Ls%=:");
    default:
      gcc_unreachable ();
    }
}

/* ------------------------------------------------------------------------- */
/* Prologue and epilogue                                                     */
/* ------------------------------------------------------------------------- */

/* Return true if the current function needs IX set up as frame pointer:
   it has locals / spill slots or receives arguments on the stack.  */

static bool
z80_frame_needed_p (void)
{
  return (get_frame_size () > 0 || crtl->args.size > 0
	  || cfun->calls_alloca);
}

/* Implement TARGET_FRAME_POINTER_REQUIRED.  There is no (sp+d) addressing
   mode, so all frame accesses go through IX.  */

static bool
z80_frame_pointer_required (void)
{
  return true;
}

/* Implement TARGET_CAN_ELIMINATE.  */

static bool
z80_can_eliminate (const int, const int to)
{
  return to == HARD_FRAME_POINTER_REGNUM;
}

/* Implement INITIAL_ELIMINATION_OFFSET.  IX points at the saved IX; the
   return address sits above it and the stack arguments above that.  The
   local variables grow downwards from IX.  */

HOST_WIDE_INT
z80_initial_elimination_offset (int from, int to)
{
  gcc_assert (to == HARD_FRAME_POINTER_REGNUM);
  if (from == ARG_POINTER_REGNUM)
    return 4;
  return 0;
}

/* Implement PUSH_ROUNDING: only 16-bit pushes exist.  */

poly_int64
z80_push_rounding (poly_int64 bytes)
{
  return (bytes.to_constant () + 1) & ~1;
}

/* Return true if the current function has the attribute NAME.  */

static bool
z80_function_attribute_p (const char *name)
{
  return (current_function_decl
	  && lookup_attribute (name, DECL_ATTRIBUTES (current_function_decl)));
}

static bool
z80_naked_function_p (void)
{
  return z80_function_attribute_p ("naked");
}

/* Interrupt handlers ("interrupt" for maskable interrupts, "nmi" for the
   non-maskable one) must preserve every register they change.  */

static bool
z80_interrupt_function_p (void)
{
  return (z80_function_attribute_p ("interrupt")
	  || z80_function_attribute_p ("nmi"));
}

/* Bits of the register-save mask used by the z80_save_regs and
   z80_restore_regs insns.  */
#define Z80_SAVE_AF 1
#define Z80_SAVE_BC 2
#define Z80_SAVE_DE 4
#define Z80_SAVE_HL 8
#define Z80_SAVE_IY 16

/* Return the registers an interrupt handler must save: those it uses, or
   all call-clobbered registers if it calls other functions.  AF and IY
   (which many insns use as scratch without saying so) are always
   saved.  */

static int
z80_interrupt_save_mask (void)
{
  int mask = Z80_SAVE_AF | Z80_SAVE_IY;
  bool calls = !crtl->is_leaf;

  if (calls || df_regs_ever_live_p (C_REG) || df_regs_ever_live_p (B_REG))
    mask |= Z80_SAVE_BC;
  if (calls || df_regs_ever_live_p (E_REG) || df_regs_ever_live_p (D_REG))
    mask |= Z80_SAVE_DE;
  if (calls || df_regs_ever_live_p (L_REG) || df_regs_ever_live_p (H_REG))
    mask |= Z80_SAVE_HL;
  return mask;
}

void
z80_expand_prologue (void)
{
  if (z80_naked_function_p ())
    return;
  if (z80_interrupt_function_p ())
    emit_insn (gen_z80_save_regs (GEN_INT (z80_interrupt_save_mask ())));
  if (z80_frame_needed_p ())
    emit_insn (gen_z80_prologue (GEN_INT (get_frame_size ())));
}

void
z80_expand_epilogue (void)
{
  if (z80_naked_function_p ())
    return;
  if (z80_frame_needed_p ())
    emit_insn (gen_z80_epilogue ());
  if (z80_interrupt_function_p ())
    {
      emit_insn (gen_z80_restore_regs (GEN_INT (z80_interrupt_save_mask ())));
      if (z80_function_attribute_p ("nmi"))
	emit_jump_insn (gen_z80_return_nmi ());
      else
	emit_jump_insn (gen_z80_return_interrupt ());
    }
  else
    emit_jump_insn (gen_z80_return ());
}

/* Output the pushes (SAVE) or pops (!SAVE) for the register-save MASK.  */

const char *
z80_output_save_regs (rtx mask_rtx, bool save)
{
  static const char *const names[] = { "af", "bc", "de", "hl", "iy" };
  int mask = INTVAL (mask_rtx);
  char buf[16];

  for (int i = 0; i < 5; i++)
    {
      int r = save ? i : 4 - i;
      if (mask & (1 << r))
	{
	  snprintf (buf, sizeof buf, "%s\t%s", save ? "push" : "pop", names[r]);
	  output_asm_insn (buf, NULL);
	}
    }
  return "";
}

/* Implement TARGET_WARN_FUNC_RETURN: a naked function returns through
   its own inline assembly.  */

static bool
z80_warn_func_return (tree decl)
{
  return !lookup_attribute ("naked", DECL_ATTRIBUTES (decl));
}

/* Implement TARGET_ALLOCATE_STACK_SLOTS_FOR_ARGS: a naked function has no
   frame to put them in.  */

static bool
z80_allocate_stack_slots_for_args (void)
{
  return !z80_naked_function_p ();
}

/* Handle the "interrupt", "nmi" and "naked" attributes: they only apply
   to functions.  */

static tree
z80_handle_fndecl_attribute (tree *node, tree name, tree, int,
			     bool *no_add_attrs)
{
  if (TREE_CODE (*node) != FUNCTION_DECL)
    {
      warning (OPT_Wattributes, "%qE attribute only applies to functions",
	       name);
      *no_add_attrs = true;
    }
  return NULL_TREE;
}

TARGET_GNU_ATTRIBUTES (z80_attribute_table,
{
  /* { name, min_len, max_len, decl_req, type_req, fn_type_req,
       affects_type_identity, handler, exclude } */
  { "interrupt", 0, 0, true, false, false, false,
    z80_handle_fndecl_attribute, NULL },
  { "nmi", 0, 0, true, false, false, false,
    z80_handle_fndecl_attribute, NULL },
  { "naked", 0, 0, true, false, false, false,
    z80_handle_fndecl_attribute, NULL }
});

/* Output a sequence that adds DELTA to SP without touching any allocatable
   register.  */

const char *
z80_output_sp_adjust (HOST_WIDE_INT delta)
{
  z80_output_add_const (stack_pointer_rtx, delta);
  return "";
}

/* Output the prologue insn.  Operand 0 is the frame size.  */

const char *
z80_output_prologue (rtx *operands)
{
  HOST_WIDE_INT size = INTVAL (operands[0]);

  output_asm_insn ("push\tix\n\tld\tix,0\n\tadd\tix,sp", operands);
  if (size > 0 && size <= 6)
    {
      for (; size >= 2; size -= 2)
	output_asm_insn ("push\taf", operands);
      if (size)
	output_asm_insn ("dec\tsp", operands);
    }
  else if (size > 0)
    z80_output_sp_adjust (-size);
  return "";
}

/* ------------------------------------------------------------------------- */
/* Function arguments and return values                                      */
/* ------------------------------------------------------------------------- */

/* Calling convention:

     - The first two 8/16-bit arguments are passed in HL (or L) and DE (or E).
     - All other arguments are pushed on the stack, right to left; byte
       arguments occupy a 2-byte slot.  The caller pops the arguments.
     - Variadic functions receive all arguments on the stack.
     - 8-bit values are returned in L, 16-bit in HL and 32-bit in DE:HL
       (DE = low word).  Larger values are returned in memory.
     - IX (frame pointer) is preserved, everything else is clobbered.  */

void
z80_init_cumulative_args (CUMULATIVE_ARGS *cum, tree fntype, rtx, tree)
{
  cum->nregs = 0;
  cum->stdarg = fntype && stdarg_p (fntype);
}

static bool
z80_arg_in_reg_p (const CUMULATIVE_ARGS *cum, const function_arg_info &arg)
{
  if (arg.end_marker_p () || !arg.named || cum->stdarg)
    return false;
  if (arg.mode == BLKmode || arg.aggregate_type_p ())
    return false;
  if (GET_MODE_SIZE (arg.mode) > 2)
    return false;
  return cum->nregs < 2;
}

/* Implement TARGET_FUNCTION_ARG.  */

static rtx
z80_function_arg (cumulative_args_t cum_v, const function_arg_info &arg)
{
  CUMULATIVE_ARGS *cum = get_cumulative_args (cum_v);

  if (!z80_arg_in_reg_p (cum, arg))
    return NULL_RTX;

  return gen_rtx_REG (arg.mode, cum->nregs == 0 ? L_REG : E_REG);
}

/* Implement TARGET_FUNCTION_ARG_ADVANCE.  */

static void
z80_function_arg_advance (cumulative_args_t cum_v,
			  const function_arg_info &arg)
{
  CUMULATIVE_ARGS *cum = get_cumulative_args (cum_v);

  if (z80_arg_in_reg_p (cum, arg))
    cum->nregs++;
}

static rtx
z80_value_reg (machine_mode mode)
{
  return gen_rtx_REG (mode, GET_MODE_SIZE (mode) <= 2 ? L_REG : E_REG);
}

/* Implement TARGET_FUNCTION_VALUE.  */

static rtx
z80_function_value (const_tree valtype, const_tree, bool)
{
  return z80_value_reg (TYPE_MODE (valtype));
}

/* Implement TARGET_LIBCALL_VALUE.  */

static rtx
z80_libcall_value (machine_mode mode, const_rtx)
{
  return z80_value_reg (mode);
}

/* Implement TARGET_FUNCTION_VALUE_REGNO_P.  */

static bool
z80_function_value_regno_p (const unsigned int regno)
{
  return regno == L_REG || regno == E_REG;
}

/* Implement TARGET_RETURN_IN_MEMORY.  */

static bool
z80_return_in_memory (const_tree type, const_tree)
{
  HOST_WIDE_INT size = int_size_in_bytes (type);
  return size == -1 || size > 4 || TYPE_MODE (type) == BLKmode;
}

/* ------------------------------------------------------------------------- */
/* Costs                                                                     */
/* ------------------------------------------------------------------------- */

/* The costs below approximate the number of instructions needed; that is a
   reasonable measure of both size and speed on the Z80.  */

/* Implement TARGET_ADDRESS_COST.  (hl) is the cheapest address, (ix+d)
   needs a prefix and a displacement byte, an absolute address can only be
   used with A or 16-bit HL/ED-prefixed loads, and a far (ix+d) needs IY to
   be set up first.  */

static int
z80_address_cost (rtx addr, machine_mode mode, addr_space_t, bool)
{
  if (REG_P (addr))
    /* (hl) is direct; (bc)/(de) are redirected through HL.  Pseudos will
       usually end up in HL.  */
    return (REGNO (addr) == C_REG || REGNO (addr) == E_REG) ? 2 : 0;
  if (GET_CODE (addr) == PLUS && CONST_INT_P (XEXP (addr, 1)))
    return (z80_short_displacement_p (INTVAL (XEXP (addr, 1)),
				      z80_access_size (mode))
	    ? 1 : 8);
  if (CONSTANT_P (addr))
    return 1;
  return 4;
}

/* Implement TARGET_RTX_COSTS.  */

static bool
z80_rtx_costs (rtx x, machine_mode mode, int outer_code, int,
	       int *total, bool speed)
{
  int size = mode == VOIDmode || mode == BLKmode ? 1 : GET_MODE_SIZE (mode);
  enum rtx_code code = GET_CODE (x);

  switch (code)
    {
    case CONST_INT:
    case CONST:
    case SYMBOL_REF:
    case LABEL_REF:
      /* Constants are immediate operands of most instructions.  */
      *total = 0;
      return true;

    case MEM:
      *total = (COSTS_N_INSNS (size)
		+ z80_address_cost (XEXP (x, 0), mode, ADDR_SPACE_GENERIC,
				    speed));
      return true;

    case PLUS:
      if (size == 1)
	*total = COSTS_N_INSNS (1);
      else if (size == 2)
	*total = (CONST_INT_P (XEXP (x, 1))
		  && IN_RANGE (INTVAL (XEXP (x, 1)), -2, 2)
		  ? COSTS_N_INSNS (1) : COSTS_N_INSNS (2));
      else
	*total = COSTS_N_INSNS (3 * size);
      return false;

    case MINUS:
      *total = COSTS_N_INSNS (size == 1 ? 1 : size == 2 ? 2 : 3 * size);
      return false;

    case AND:
    case IOR:
    case XOR:
      /* Through A, one byte at a time.  */
      *total = COSTS_N_INSNS (size == 1 ? 1 : 3 * size);
      return false;

    case NEG:
    case NOT:
      *total = COSTS_N_INSNS (size == 1 ? 1 : 3 * size);
      return false;

    case MULT:
      /* Library calls.  */
      *total = COSTS_N_INSNS (speed ? 30 * size : 4 + size);
      return true;

    case DIV:
    case UDIV:
    case MOD:
    case UMOD:
      *total = COSTS_N_INSNS (speed ? 60 * size : 4 + size);
      return true;

    case ASHIFT:
    case LSHIFTRT:
    case ASHIFTRT:
      if (size > 2)
	*total = COSTS_N_INSNS (6);
      else if (CONST_INT_P (XEXP (x, 1)))
	{
	  int count = INTVAL (XEXP (x, 1)) & (8 * size - 1);
	  if (size == 2 && count >= 8)
	    count = 2 + (count - 8);
	  *total = COSTS_N_INSNS (count * size);
	}
      else
	*total = COSTS_N_INSNS (6 + 2 * size);
      return false;

    case ZERO_EXTEND:
      *total = COSTS_N_INSNS (1);
      return false;

    case SIGN_EXTEND:
      *total = COSTS_N_INSNS (4);
      return false;

    case COMPARE:
      *total = COSTS_N_INSNS (size == 1 ? 1 : 3);
      return false;

    default:
      return false;
    }
}

/* ------------------------------------------------------------------------- */
/* Types                                                                     */
/* ------------------------------------------------------------------------- */

/* Implement TARGET_C_MODE_FOR_FLOATING_TYPE.  double and long double are
   32-bit (the same format as float), as usual for 8-bit targets: 64-bit
   soft float is too big and slow to be useful on a Z80.  */

static machine_mode
z80_c_mode_for_floating_type (enum tree_index ti)
{
  if (ti == TI_DOUBLE_TYPE || ti == TI_LONG_DOUBLE_TYPE)
    return SFmode;
  return default_mode_for_floating_type (ti);
}

/* Implement TARGET_SCALAR_MODE_SUPPORTED_P.  DFmode is no longer the mode
   of any C type, but keep it available (__attribute__ ((mode (DF))), with
   the soft-float routines in libgcc).  */

static bool
z80_scalar_mode_supported_p (scalar_mode mode)
{
  if (mode == DFmode)
    return true;
  return default_scalar_mode_supported_p (mode);
}

/* ------------------------------------------------------------------------- */
/* Target hooks                                                              */
/* ------------------------------------------------------------------------- */

#undef TARGET_HARD_REGNO_NREGS
#define TARGET_HARD_REGNO_NREGS z80_hard_regno_nregs
#undef TARGET_HARD_REGNO_MODE_OK
#define TARGET_HARD_REGNO_MODE_OK z80_hard_regno_mode_ok
#undef TARGET_MODES_TIEABLE_P
#define TARGET_MODES_TIEABLE_P z80_modes_tieable_p
#undef TARGET_CLASS_LIKELY_SPILLED_P
#define TARGET_CLASS_LIKELY_SPILLED_P z80_class_likely_spilled_p

#undef TARGET_LEGITIMATE_ADDRESS_P
#define TARGET_LEGITIMATE_ADDRESS_P z80_legitimate_address_p

#undef TARGET_ADDRESS_COST
#define TARGET_ADDRESS_COST z80_address_cost
#undef TARGET_RTX_COSTS
#define TARGET_RTX_COSTS z80_rtx_costs

#undef TARGET_PRINT_OPERAND
#define TARGET_PRINT_OPERAND z80_print_operand
#undef TARGET_ASM_FINAL_POSTSCAN_INSN
#define TARGET_ASM_FINAL_POSTSCAN_INSN z80_final_postscan_insn

#undef TARGET_PRINT_OPERAND_ADDRESS
#define TARGET_PRINT_OPERAND_ADDRESS z80_print_operand_address

#undef TARGET_ATTRIBUTE_TABLE
#define TARGET_ATTRIBUTE_TABLE z80_attribute_table
#undef TARGET_WARN_FUNC_RETURN
#define TARGET_WARN_FUNC_RETURN z80_warn_func_return
#undef TARGET_ALLOCATE_STACK_SLOTS_FOR_ARGS
#define TARGET_ALLOCATE_STACK_SLOTS_FOR_ARGS z80_allocate_stack_slots_for_args

#undef TARGET_FRAME_POINTER_REQUIRED
#define TARGET_FRAME_POINTER_REQUIRED z80_frame_pointer_required
#undef TARGET_CAN_ELIMINATE
#define TARGET_CAN_ELIMINATE z80_can_eliminate

#undef TARGET_FUNCTION_ARG
#define TARGET_FUNCTION_ARG z80_function_arg
#undef TARGET_FUNCTION_ARG_ADVANCE
#define TARGET_FUNCTION_ARG_ADVANCE z80_function_arg_advance
#undef TARGET_FUNCTION_VALUE
#define TARGET_FUNCTION_VALUE z80_function_value
#undef TARGET_LIBCALL_VALUE
#define TARGET_LIBCALL_VALUE z80_libcall_value
#undef TARGET_FUNCTION_VALUE_REGNO_P
#define TARGET_FUNCTION_VALUE_REGNO_P z80_function_value_regno_p
#undef TARGET_RETURN_IN_MEMORY
#define TARGET_RETURN_IN_MEMORY z80_return_in_memory

#undef TARGET_C_MODE_FOR_FLOATING_TYPE
#define TARGET_C_MODE_FOR_FLOATING_TYPE z80_c_mode_for_floating_type
#undef TARGET_SCALAR_MODE_SUPPORTED_P
#define TARGET_SCALAR_MODE_SUPPORTED_P z80_scalar_mode_supported_p

/* Constructors and destructors go into .init_array / .fini_array, which
   crt0 walks (there are no crtbegin/crtend files).  */
#undef TARGET_ASM_CONSTRUCTOR
#define TARGET_ASM_CONSTRUCTOR default_elf_init_array_asm_out_constructor
#undef TARGET_ASM_DESTRUCTOR
#define TARGET_ASM_DESTRUCTOR default_elf_fini_array_asm_out_destructor

#undef TARGET_ASM_ALIGNED_HI_OP
#define TARGET_ASM_ALIGNED_HI_OP "\t.word\t"
#undef TARGET_ASM_UNALIGNED_HI_OP
#define TARGET_ASM_UNALIGNED_HI_OP "\t.word\t"
#undef TARGET_ASM_ALIGNED_SI_OP
#define TARGET_ASM_ALIGNED_SI_OP "\t.long\t"
#undef TARGET_ASM_UNALIGNED_SI_OP
#define TARGET_ASM_UNALIGNED_SI_OP "\t.long\t"

struct gcc_target targetm = TARGET_INITIALIZER;
