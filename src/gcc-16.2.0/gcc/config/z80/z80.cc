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

/* Return true if REGNO can be used as a base register: HL, IX, or one of
   the soft frame/argument pointers (which are eliminated into IX).  */

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

  return (regno == L_REG || regno == IXL_REG || regno == FP_REG
	  || regno == AP_REG);
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

/* Return true if OP is a memory reference that every 8-bit register can
   use directly, i.e. its address is (hl) or (ix+d).  Used by the "Q"
   constraint.  */

bool
z80_q_memory_p (rtx op)
{
  if (!MEM_P (op))
    return false;

  rtx addr = XEXP (op, 0);

  if (REG_P (addr))
    {
      /* Before register allocation a pseudo address will be allocated to
	 the base register class (HL).  */
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
z80_final_prescan_insn (rtx_insn *, rtx *operands, int noperands)
{
  z80_far_addr = NULL_RTX;

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
      if (regno == L_REG && disp == 0)
	fputs ("(hl)", file);
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
	unsigned int regno = REGNO (x);
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
	else if (GET_MODE_SIZE (GET_MODE (x)) <= 2)
	  val &= 0xffff;
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

/* Output an 8-bit move.  Every alternative is a single "ld".  */

const char *
z80_output_movqi (rtx *operands)
{
  if (REG_P (operands[0]) && REG_P (operands[1])
      && REGNO (operands[0]) == REGNO (operands[1]))
    return "";
  return "ld\t%0,%1";
}

static bool
z80_hl_reg_p (rtx x)
{
  return REG_P (x) && REGNO (x) == L_REG;
}

static bool
z80_ix_memory_p (rtx mem)
{
  rtx addr = XEXP (mem, 0);
  if (GET_CODE (addr) == PLUS)
    addr = XEXP (addr, 0);
  return REG_P (addr) && REGNO (addr) == IXL_REG;
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
	{
	  /* Load the high byte last: the destination may not overlap
	     the address register (IX), so the order does not matter.  */
	  return "ld\t%L0,%L1\n\tld\t%H0,%H1";
	}

      /* (hl) */
      gcc_assert (REG_P (addr) && REGNO (addr) == L_REG);
      if (d == L_REG)
	return ("push\tde\n\tld\te,(hl)\n\tinc\thl\n\tld\td,(hl)\n\t"
		"ex\tde,hl\n\tpop\tde");
      return "ld\t%L0,(hl)\n\tinc\thl\n\tld\t%H0,(hl)\n\tdec\thl";
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

  /* (hl) */
  gcc_assert (REG_P (addr) && REGNO (addr) == L_REG);
  if (z80_hl_reg_p (src))
    return ("push\tde\n\tld\te,l\n\tld\td,h\n\tld\t(hl),e\n\tinc\thl\n\t"
	    "ld\t(hl),d\n\tdec\thl\n\tpop\tde");
  return "ld\t(hl),%L1\n\tinc\thl\n\tld\t(hl),%H1\n\tdec\thl";
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

  if (GET_MODE (operands[1]) == QImode)
    output_asm_insn ("cp\t%2", operands);
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

void
z80_expand_prologue (void)
{
  if (z80_frame_needed_p ())
    emit_insn (gen_z80_prologue (GEN_INT (get_frame_size ())));
}

void
z80_expand_epilogue (void)
{
  if (z80_frame_needed_p ())
    emit_insn (gen_z80_epilogue ());
  emit_jump_insn (gen_z80_return ());
}

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
/* Target hooks                                                              */
/* ------------------------------------------------------------------------- */

#undef TARGET_HARD_REGNO_NREGS
#define TARGET_HARD_REGNO_NREGS z80_hard_regno_nregs
#undef TARGET_HARD_REGNO_MODE_OK
#define TARGET_HARD_REGNO_MODE_OK z80_hard_regno_mode_ok
#undef TARGET_MODES_TIEABLE_P
#define TARGET_MODES_TIEABLE_P z80_modes_tieable_p

#undef TARGET_LEGITIMATE_ADDRESS_P
#define TARGET_LEGITIMATE_ADDRESS_P z80_legitimate_address_p

#undef TARGET_PRINT_OPERAND
#define TARGET_PRINT_OPERAND z80_print_operand
#undef TARGET_PRINT_OPERAND_ADDRESS
#define TARGET_PRINT_OPERAND_ADDRESS z80_print_operand_address

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

#undef TARGET_ASM_ALIGNED_HI_OP
#define TARGET_ASM_ALIGNED_HI_OP "\t.word\t"
#undef TARGET_ASM_UNALIGNED_HI_OP
#define TARGET_ASM_UNALIGNED_HI_OP "\t.word\t"
#undef TARGET_ASM_ALIGNED_SI_OP
#define TARGET_ASM_ALIGNED_SI_OP "\t.long\t"
#undef TARGET_ASM_UNALIGNED_SI_OP
#define TARGET_ASM_UNALIGNED_SI_OP "\t.long\t"

struct gcc_target targetm = TARGET_INITIALIZER;
