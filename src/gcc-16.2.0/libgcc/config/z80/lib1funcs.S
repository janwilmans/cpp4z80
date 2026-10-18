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

	.text

; ---------------------------------------------------------------------------
; __mulhi3:  HL = HL * DE   (also correct for signed operands)
; ---------------------------------------------------------------------------
	.globl	__mulhi3
__mulhi3:
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

; __mulqi3:  L = L * E
	.globl	__mulqi3
__mulqi3:
	ld	h,0
	ld	d,0
	jr	__mulhi3

; ---------------------------------------------------------------------------
; __udivmodhi_core:  unsigned 16-bit division
;   in:   HL = dividend, DE = divisor
;   out:  BC = quotient, HL = remainder
; Restoring shift-and-subtract division, one quotient bit per iteration.
; ---------------------------------------------------------------------------
__udivmodhi_core:
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

; __udivhi3:  HL = HL / DE (unsigned)
	.globl	__udivhi3
__udivhi3:
	call	__udivmodhi_core
	ld	h,b
	ld	l,c
	ret

; __umodhi3:  HL = HL % DE (unsigned)
	.globl	__umodhi3
__umodhi3:
	jr	__udivmodhi_core

; __udivqi3:  L = L / E (unsigned)
	.globl	__udivqi3
__udivqi3:
	ld	h,0
	ld	d,0
	jr	__udivhi3

; __umodqi3:  L = L % E (unsigned)
	.globl	__umodqi3
__umodqi3:
	ld	h,0
	ld	d,0
	jr	__udivmodhi_core

; ---------------------------------------------------------------------------
; Signed division: divide the magnitudes, then fix the sign.
; The quotient is negative if the operand signs differ; the remainder has
; the sign of the dividend (C99 truncating division).
; ---------------------------------------------------------------------------

; __neghl:  HL = -HL
__neghl:
	xor	a
	sub	l
	ld	l,a
	sbc	a,a
	sub	h
	ld	h,a
	ret

; __abshl:  HL = |HL|
__abshl:
	bit	7,h
	ret	z
	jr	__neghl

; __divhi3:  HL = HL / DE (signed)
	.globl	__divhi3
__divhi3:
	ld	a,h
	xor	d		; S = sign of the quotient
	push	af
	call	__abshl
	ex	de,hl
	call	__abshl
	ex	de,hl
	call	__udivmodhi_core
	ld	h,b
	ld	l,c
	pop	af
	ret	p
	jr	__neghl

; __modhi3:  HL = HL % DE (signed)
	.globl	__modhi3
__modhi3:
	ld	a,h
	or	a		; S = sign of the dividend
	push	af
	call	__abshl
	ex	de,hl
	call	__abshl
	ex	de,hl
	call	__udivmodhi_core
	pop	af
	ret	p
	jr	__neghl

; Sign-extend the 8-bit arguments L and E and use the 16-bit routines.
	.globl	__divqi3
__divqi3:
	ld	a,l
	rla
	sbc	a,a
	ld	h,a
	ld	a,e
	rla
	sbc	a,a
	ld	d,a
	jr	__divhi3

	.globl	__modqi3
__modqi3:
	ld	a,l
	rla
	sbc	a,a
	ld	h,a
	ld	a,e
	rla
	sbc	a,a
	ld	d,a
	jr	__modhi3

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

; __neg32:  DE:HL = -DE:HL
__neg32:
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

; __neg32_mem:  negate the 32-bit value at (hl)
__neg32_mem:
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

; ---------------------------------------------------------------------------
; __mulsi3:  a * b  (also correct for signed operands)
; Shift-and-add over the 32 bits of b, most significant bit first.
; ---------------------------------------------------------------------------
	.globl	__mulsi3
__mulsi3:
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

; ---------------------------------------------------------------------------
; __udivmodsi_core:  unsigned 32-bit division, IX frame already set up
;   in:   (ix+4..7) dividend, (ix+8..11) divisor
;   out:  (ix+4..7) quotient, DE:HL remainder
; ---------------------------------------------------------------------------
__udivmodsi_core:
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
__load_quotient:
	ld	e,(ix+4)
	ld	d,(ix+5)
	ld	l,(ix+6)
	ld	h,(ix+7)
	ret

	.globl	__udivsi3
__udivsi3:
	push	ix
	ld	ix,0
	add	ix,sp
	call	__udivmodsi_core
	call	__load_quotient
	pop	ix
	ret

	.globl	__umodsi3
__umodsi3:
	push	ix
	ld	ix,0
	add	ix,sp
	call	__udivmodsi_core
	pop	ix
	ret

; Make both arguments non-negative (in place).
__abs_args32:
	bit	7,(ix+7)
	jr	z,.Labs32_b
	push	ix
	pop	hl
	ld	de,4
	add	hl,de
	call	__neg32_mem
.Labs32_b:
	bit	7,(ix+11)
	ret	z
	push	ix
	pop	hl
	ld	de,8
	add	hl,de
	jp	__neg32_mem

	.globl	__divsi3
__divsi3:
	push	ix
	ld	ix,0
	add	ix,sp
	ld	a,(ix+7)
	xor	(ix+11)		; S = sign of the quotient
	push	af
	call	__abs_args32
	call	__udivmodsi_core
	call	__load_quotient
	pop	af
	pop	ix
	ret	p
	jp	__neg32

	.globl	__modsi3
__modsi3:
	push	ix
	ld	ix,0
	add	ix,sp
	ld	a,(ix+7)
	or	a		; S = sign of the dividend
	push	af
	call	__abs_args32
	call	__udivmodsi_core
	pop	af
	pop	ix
	ret	p
	jp	__neg32

; ---------------------------------------------------------------------------
; 32-bit shifts: value at (sp+2), count in L, result in DE:HL.
; ---------------------------------------------------------------------------

; Load the value into DE:HL and the count (masked to 0..31) into A/B.
; Returns with Z set if the count is zero.
__shift_args32:
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

	.globl	__ashlsi3
__ashlsi3:
	call	__shift_args32
	ret	z
.Lashl32:
	sla	e
	rl	d
	adc	hl,hl
	djnz	.Lashl32
	ret

	.globl	__lshrsi3
__lshrsi3:
	call	__shift_args32
	ret	z
.Llshr32:
	srl	h
	rr	l
	rr	d
	rr	e
	djnz	.Llshr32
	ret

	.globl	__ashrsi3
__ashrsi3:
	call	__shift_args32
	ret	z
.Lashr32:
	sra	h
	rr	l
	rr	d
	rr	e
	djnz	.Lashr32
	ret
