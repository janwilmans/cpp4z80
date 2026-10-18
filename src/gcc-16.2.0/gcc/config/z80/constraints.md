;; Z80 constraints.
;; Copyright (C) 2026 Free Software Foundation, Inc.
;;
;; Register numbering and classes are defined in z80.h.
;;
;; QImode registers:   A B C D E H L
;; HImode pairs:       BC DE HL (allocatable), IX (frame pointer),
;;                     IY (scratch), SP
;;
;; The generic constraints "r" (GENERAL_REGS), "m", "n", "i", "X" etc. are
;; provided by GCC itself and must not be redefined here.

;; =========================================================================
;; Register constraints
;; =========================================================================

(define_register_constraint "a" "A_REGS"
  "The accumulator A.")

(define_register_constraint "b" "BC_REGS"
  "The BC register pair (or one of B, C).")

(define_register_constraint "d" "DE_REGS"
  "The DE register pair (or one of D, E).")

(define_register_constraint "h" "HL_REGS"
  "The HL register pair (or one of H, L).")

(define_register_constraint "w" "BCDE_REGS"
  "The BC or DE register pair.")

(define_register_constraint "l" "LOW_REGS"
  "The low half of a register pair: C, E or L.")

(define_register_constraint "x" "IX_REGS"
  "The IX index register (the hard frame pointer).")

(define_register_constraint "y" "IY_REGS"
  "The IY index register.")

(define_register_constraint "k" "SP_REGS"
  "The stack pointer SP.")

;; =========================================================================
;; Memory constraints
;; =========================================================================

;; Memory that every 8-bit register can be loaded from / stored to:
;; (hl) or (ix+d).
(define_memory_constraint "Q"
  "A memory operand addressed by HL or IX+d."
  (and (match_code "mem")
       (match_test "z80_q_memory_p (op)")))

;; Memory at a constant address (nn).  Only A can be transferred to/from
;; such an address in QImode.  This is deliberately NOT a memory constraint:
;; reloading the address into a register never makes it satisfy "S".
(define_constraint "S"
  "A memory operand with a constant address."
  (and (match_code "mem")
       (match_test "z80_abs_memory_p (op)")))

;; =========================================================================
;; Integer constraints
;; =========================================================================

(define_constraint "I"
  "The constant 1."
  (and (match_code "const_int")
       (match_test "ival == 1")))

(define_constraint "J"
  "The constant -1."
  (and (match_code "const_int")
       (match_test "ival == -1")))

(define_constraint "L"
  "A small constant (-4..4) that is cheap to add with inc/dec."
  (and (match_code "const_int")
       (match_test "ival >= -4 && ival <= 4")))

(define_constraint "P"
  "The constant 0."
  (and (match_code "const_int")
       (match_test "ival == 0")))
