; Z80 support routines for libgcc: integer multiply and divide.
; Copyright (C) 2026 Free Software Foundation, Inc.
;
; This file is part of GCC.
;
; GCC is free software; you can redistribute it and/or modify it under
; the terms of the GNU General Public License as published by the Free
; Software Foundation; either version 3, or (at your option) any later
; version.
;
; Calling convention (see gcc/config/z80/z80.cc):
;   first argument  in L / HL, second argument in E / DE
;   result          in L / HL
;   only IX must be preserved.
;
; C symbols get a "_" prefix in assembly (USER_LABEL_PREFIX), so the
; routine GCC calls as __mulhi3 is ___mulhi3 here.  The __z80_* helpers
; are only used from assembly.
;
; libgcc assembles this file once per function listed in LIB1ASMFUNCS
; (see t-z80), with L_<function> defined.  Helpers shared between these
; objects have global __z80_ names.

	.text

#ifdef L_mulhi3
; ---------------------------------------------------------------------------
; ___mulhi3:  HL = HL * DE   (also correct for signed operands)
; ---------------------------------------------------------------------------
	.globl	___mulhi3
___mulhi3:
	ld	b,h
	ld	c,l		; BC = multiplicand
	ld	hl,0
	ld	a,16
.Lmul_loop:
	add	hl,hl		; result <<= 1
	sla	e
	rl	d		; next multiplier bit (MSB first) -> carry
	jr	nc,.Lmul_skip
	add	hl,bc
.Lmul_skip:
	dec	a
	jr	nz,.Lmul_loop
	ret

; ___mulqi3:  L = L * E
	.globl	___mulqi3
___mulqi3:
	ld	h,0
	ld	d,0
	jr	___mulhi3
#endif /* L_mulhi3 */

#ifdef L_udivmodhi4
; ---------------------------------------------------------------------------
; __z80_udivmodhi:  unsigned 16-bit division
;   in:   HL = dividend, DE = divisor
;   out:  BC = quotient, HL = remainder
; Restoring shift-and-subtract division, one quotient bit per iteration.
; ---------------------------------------------------------------------------
	.globl	__z80_udivmodhi
__z80_udivmodhi:
	ld	b,h
	ld	c,l		; BC = dividend, shifted into the remainder
	ld	hl,0		; HL = remainder
	ld	a,16
.Ldiv_loop:
	sla	c
	rl	b		; top dividend bit -> carry
	adc	hl,hl		; remainder = remainder * 2 + bit
	jr	c,.Ldiv_force	; remainder >= 65536 > divisor: must subtract
	or	a
	sbc	hl,de
	jr	nc,.Ldiv_set	; remainder >= divisor
	add	hl,de		; too small: restore
	jr	.Ldiv_next
.Ldiv_force:
	or	a
	sbc	hl,de
.Ldiv_set:
	inc	c		; quotient bit = 1
.Ldiv_next:
	dec	a
	jr	nz,.Ldiv_loop
	ret

; ___udivhi3:  HL = HL / DE (unsigned)
	.globl	___udivhi3
___udivhi3:
	call	__z80_udivmodhi
	ld	h,b
	ld	l,c
	ret

; ___umodhi3:  HL = HL % DE (unsigned)
	.globl	___umodhi3
___umodhi3:
	jr	__z80_udivmodhi

; ___udivqi3:  L = L / E (unsigned)
	.globl	___udivqi3
___udivqi3:
	ld	h,0
	ld	d,0
	jr	___udivhi3

; ___umodqi3:  L = L % E (unsigned)
	.globl	___umodqi3
___umodqi3:
	ld	h,0
	ld	d,0
	jr	__z80_udivmodhi
#endif /* L_udivmodhi4 */

#ifdef L_divmodhi4
; ---------------------------------------------------------------------------
; Signed division: divide the magnitudes, then fix the sign.
; The quotient is negative if the operand signs differ; the remainder has
; the sign of the dividend (C99 truncating division).
; ---------------------------------------------------------------------------

; __z80_neghl:  HL = -HL
	.globl	__z80_neghl
__z80_neghl:
	xor	a
	sub	l
	ld	l,a
	sbc	a,a
	sub	h
	ld	h,a
	ret

; __z80_abshl:  HL = |HL|
	.globl	__z80_abshl
__z80_abshl:
	bit	7,h
	ret	z
	jr	__z80_neghl

; ___divhi3:  HL = HL / DE (signed)
	.globl	___divhi3
___divhi3:
	ld	a,h
	xor	d		; S = sign of the quotient
	push	af
	call	__z80_abshl
	ex	de,hl
	call	__z80_abshl
	ex	de,hl
	call	__z80_udivmodhi
	ld	h,b
	ld	l,c
	pop	af
	ret	p
	jr	__z80_neghl

; ___modhi3:  HL = HL % DE (signed)
	.globl	___modhi3
___modhi3:
	ld	a,h
	or	a		; S = sign of the dividend
	push	af
	call	__z80_abshl
	ex	de,hl
	call	__z80_abshl
	ex	de,hl
	call	__z80_udivmodhi
	pop	af
	ret	p
	jr	__z80_neghl

; Sign-extend the 8-bit arguments L and E and use the 16-bit routines.
	.globl	___divqi3
___divqi3:
	ld	a,l
	rla
	sbc	a,a
	ld	h,a
	ld	a,e
	rla
	sbc	a,a
	ld	d,a
	jr	___divhi3

	.globl	___modqi3
___modqi3:
	ld	a,l
	rla
	sbc	a,a
	ld	h,a
	ld	a,e
	rla
	sbc	a,a
	ld	d,a
	jr	___modhi3
#endif /* L_divmodhi4 */

; ===========================================================================
; 32-bit routines.
;
; 32-bit arguments are passed on the stack; at entry (sp+2) holds the first
; argument and (sp+6) the second.  A following 8/16-bit argument (the shift
; count) is passed in L.  32-bit results are returned in DE:HL, with DE
; holding the low word.
;
; The routines that need a frame set up IX like compiled code does:
;   (ix+4..7) first argument, (ix+8..11) second argument.
; ===========================================================================
#ifdef L_divmodsi4

; __z80_neg32:  DE:HL = -DE:HL
	.globl	__z80_neg32
__z80_neg32:
	xor	a
	sub	e
	ld	e,a
	ld	a,0
	sbc	a,d
	ld	d,a
	ld	a,0
	sbc	a,l
	ld	l,a
	ld	a,0
	sbc	a,h
	ld	h,a
	ret

; __z80_neg32_mem:  negate the 32-bit value at (hl)
	.globl	__z80_neg32_mem
__z80_neg32_mem:
	xor	a
	sub	(hl)
	ld	(hl),a
	inc	hl
	ld	a,0
	sbc	a,(hl)
	ld	(hl),a
	inc	hl
	ld	a,0
	sbc	a,(hl)
	ld	(hl),a
	inc	hl
	ld	a,0
	sbc	a,(hl)
	ld	(hl),a
	ret
#endif /* L_divmodsi4 */

#ifdef L_mulsi3
; ---------------------------------------------------------------------------
; ___mulsi3:  a * b  (also correct for signed operands)
; Shift-and-add over the 32 bits of b, most significant bit first.
; ---------------------------------------------------------------------------
	.globl	___mulsi3
___mulsi3:
	push	ix
	ld	ix,0
	add	ix,sp
	ld	hl,0
	ld	de,0		; result DE:HL = 0
	ld	b,32
.Lmul32_loop:
	sla	e		; result <<= 1
	rl	d
	adc	hl,hl
	sla	(ix+8)		; next bit of b -> carry
	rl	(ix+9)
	rl	(ix+10)
	rl	(ix+11)
	jr	nc,.Lmul32_skip
	ld	a,e		; result += a
	add	a,(ix+4)
	ld	e,a
	ld	a,d
	adc	a,(ix+5)
	ld	d,a
	ld	a,l
	adc	a,(ix+6)
	ld	l,a
	ld	a,h
	adc	a,(ix+7)
	ld	h,a
.Lmul32_skip:
	djnz	.Lmul32_loop
	pop	ix
	ret
#endif /* L_mulsi3 */

#ifdef L_udivmodsi4
; ---------------------------------------------------------------------------
; __z80_udivmodsi:  unsigned 32-bit division, IX frame already set up
;   in:   (ix+4..7) dividend, (ix+8..11) divisor
;   out:  (ix+4..7) quotient, DE:HL remainder
; ---------------------------------------------------------------------------
	.globl	__z80_udivmodsi
__z80_udivmodsi:
	ld	hl,0
	ld	de,0		; remainder DE:HL = 0
	ld	b,32
.Ldiv32_loop:
	sla	(ix+4)		; top dividend bit -> carry
	rl	(ix+5)
	rl	(ix+6)
	rl	(ix+7)
	rl	e		; remainder = remainder * 2 + bit
	rl	d
	adc	hl,hl
	push	af		; carry: remainder overflowed 32 bits
	ld	a,e		; remainder -= divisor
	sub	(ix+8)
	ld	e,a
	ld	a,d
	sbc	a,(ix+9)
	ld	d,a
	ld	a,l
	sbc	a,(ix+10)
	ld	l,a
	ld	a,h
	sbc	a,(ix+11)
	ld	h,a
	jr	nc,.Ldiv32_fits
	pop	af		; borrow: only fits if the remainder overflowed
	jr	c,.Ldiv32_set
	ld	a,e		; does not fit: restore
	add	a,(ix+8)
	ld	e,a
	ld	a,d
	adc	a,(ix+9)
	ld	d,a
	ld	a,l
	adc	a,(ix+10)
	ld	l,a
	ld	a,h
	adc	a,(ix+11)
	ld	h,a
	jr	.Ldiv32_next
.Ldiv32_fits:
	pop	af
.Ldiv32_set:
	inc	(ix+4)		; quotient bit = 1
.Ldiv32_next:
	djnz	.Ldiv32_loop
	ret

; Load the quotient from (ix+4..7) into DE:HL.
	.globl	__z80_load_quotient
__z80_load_quotient:
	ld	e,(ix+4)
	ld	d,(ix+5)
	ld	l,(ix+6)
	ld	h,(ix+7)
	ret

	.globl	___udivsi3
___udivsi3:
	push	ix
	ld	ix,0
	add	ix,sp
	call	__z80_udivmodsi
	call	__z80_load_quotient
	pop	ix
	ret

	.globl	___umodsi3
___umodsi3:
	push	ix
	ld	ix,0
	add	ix,sp
	call	__z80_udivmodsi
	pop	ix
	ret
#endif /* L_udivmodsi4 */

#ifdef L_divmodsi4
; Make both arguments non-negative (in place).
	.globl	__z80_abs_args32
__z80_abs_args32:
	bit	7,(ix+7)
	jr	z,.Labs32_b
	push	ix
	pop	hl
	ld	de,4
	add	hl,de
	call	__z80_neg32_mem
.Labs32_b:
	bit	7,(ix+11)
	ret	z
	push	ix
	pop	hl
	ld	de,8
	add	hl,de
	jp	__z80_neg32_mem

	.globl	___divsi3
___divsi3:
	push	ix
	ld	ix,0
	add	ix,sp
	ld	a,(ix+7)
	xor	(ix+11)		; S = sign of the quotient
	push	af
	call	__z80_abs_args32
	call	__z80_udivmodsi
	call	__z80_load_quotient
	pop	af
	pop	ix
	ret	p
	jp	__z80_neg32

	.globl	___modsi3
___modsi3:
	push	ix
	ld	ix,0
	add	ix,sp
	ld	a,(ix+7)
	or	a		; S = sign of the dividend
	push	af
	call	__z80_abs_args32
	call	__z80_udivmodsi
	pop	af
	pop	ix
	ret	p
	jp	__z80_neg32
#endif /* L_divmodsi4 */

#ifdef L_shiftsi3
; ---------------------------------------------------------------------------
; 32-bit shifts: value at (sp+2), count in L, result in DE:HL.
; ---------------------------------------------------------------------------

; Load the value into DE:HL and the count (masked to 0..31) into A/B.
; Returns with Z set if the count is zero.
	.globl	__z80_shift_args32
__z80_shift_args32:
	ld	b,l
	ld	hl,4		; skip our own return address too
	add	hl,sp
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a
	ld	a,b
	and	31
	ld	b,a
	ret

	.globl	___ashlsi3
___ashlsi3:
	call	__z80_shift_args32
	ret	z
.Lashl32:
	sla	e
	rl	d
	adc	hl,hl
	djnz	.Lashl32
	ret

	.globl	___lshrsi3
___lshrsi3:
	call	__z80_shift_args32
	ret	z
.Llshr32:
	srl	h
	rr	l
	rr	d
	rr	e
	djnz	.Llshr32
	ret

	.globl	___ashrsi3
___ashrsi3:
	call	__z80_shift_args32
	ret	z
.Lashr32:
	sra	h
	rr	l
	rr	d
	rr	e
	djnz	.Lashr32
	ret
#endif /* L_shiftsi3 */

#ifdef L_bitopshi
; ---------------------------------------------------------------------------
; 16-bit bit operations (int is 16 bits; libgcc2 only provides the 32- and
; 64-bit variants).  Argument in HL, result in HL.
; ---------------------------------------------------------------------------

; ___clzhi2:  number of leading zero bits (16 for 0)
	.globl	___clzhi2
___clzhi2:
	ld	e,0
	ld	a,h
	or	l
	jr	z,.Lclz_zero
.Lclz_loop:
	bit	7,h
	jr	nz,.Lclz_done
	add	hl,hl
	inc	e
	jr	.Lclz_loop
.Lclz_zero:
	ld	e,16
.Lclz_done:
	ld	l,e
	ld	h,0
	ret

; ___ctzhi2:  number of trailing zero bits (16 for 0)
	.globl	___ctzhi2
___ctzhi2:
	ld	e,0
	ld	a,h
	or	l
	jr	z,.Lctz_zero
.Lctz_loop:
	bit	0,l
	jr	nz,.Lctz_done
	srl	h
	rr	l
	inc	e
	jr	.Lctz_loop
.Lctz_zero:
	ld	e,16
.Lctz_done:
	ld	l,e
	ld	h,0
	ret

; ___ffshi2:  1 + index of the least significant set bit, 0 for 0
	.globl	___ffshi2
___ffshi2:
	ld	a,h
	or	l
	ret	z
	call	___ctzhi2
	inc	hl
	ret

; ___popcounthi2:  number of set bits
	.globl	___popcounthi2
___popcounthi2:
	ld	e,0
.Lpop_loop:
	ld	a,h
	or	l
	jr	z,.Lpop_done
	srl	h
	rr	l
	jr	nc,.Lpop_loop
	inc	e
	jr	.Lpop_loop
.Lpop_done:
	ld	l,e
	ld	h,0
	ret

; ___parityhi2:  1 if the number of set bits is odd
	.globl	___parityhi2
___parityhi2:
	call	___popcounthi2
	ld	a,l
	and	1
	ld	l,a
	ret

; ___clrsbhi2:  number of leading redundant sign bits
	.globl	___clrsbhi2
___clrsbhi2:
	bit	7,h
	jr	z,.Lclrsb_pos
	ld	a,h		; negative: count the leading ones
	cpl
	ld	h,a
	ld	a,l
	cpl
	ld	l,a
.Lclrsb_pos:
	call	___clzhi2
	dec	hl
	ret
#endif /* L_bitopshi */
