; setjmp / longjmp for the Z80 (GCC calling convention).
;
; Arguments: the first in HL, the second in DE; the result in HL.
; Only IX is preserved across calls, so a jmp_buf holds three words:
;   [0] return address, [1] SP after returning from setjmp, [2] IX.
; C symbols get a "_" prefix in assembly.

	.text

; int setjmp (jmp_buf env)
	.globl	_setjmp
_setjmp:
	pop	bc		; BC = return address, SP = caller's SP
	ld	(hl),c
	inc	hl
	ld	(hl),b
	inc	hl
	ex	de,hl		; DE = &env[1]
	ld	hl,0
	add	hl,sp
	ex	de,hl		; HL = &env[1], DE = SP
	ld	(hl),e
	inc	hl
	ld	(hl),d
	inc	hl
	push	ix
	pop	de
	ld	(hl),e
	inc	hl
	ld	(hl),d
	ld	hl,0		; direct return: 0
	push	bc
	ret

; void longjmp (jmp_buf env, int val)
	.globl	_longjmp
_longjmp:
	ld	a,d
	or	e
	jr	nz,.Lnonzero
	inc	de		; longjmp (env, 0) makes setjmp return 1
.Lnonzero:
	ld	c,(hl)
	inc	hl
	ld	b,(hl)		; BC = return address
	inc	hl
	push	de		; save val
	ld	e,(hl)
	inc	hl
	ld	d,(hl)		; DE = saved SP
	inc	hl
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a
	push	hl
	pop	ix		; IX = saved IX
	pop	hl		; HL = val
	ex	de,hl
	ld	sp,hl		; SP = saved SP
	ex	de,hl		; HL = val
	push	bc
	ret
