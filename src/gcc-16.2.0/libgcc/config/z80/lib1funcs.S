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
