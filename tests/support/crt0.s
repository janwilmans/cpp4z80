; Minimal start-up code for the test programs.
;
; Sets up the stack at the top of memory, runs the C++ global constructors,
; calls main() and halts.  The test harness reads main's return value from
; HL after the HALT.

	.section .text.crt0,"ax"
	.globl	_start
_start:
	ld	sp,0

	; .ctors entries run from last to first.
	ld	hl,__ctors_end
.Lctors:
	ld	de,__ctors_start
	or	a
	sbc	hl,de
	add	hl,de
	jr	z,.Linit_array
	dec	hl
	ld	d,(hl)
	dec	hl
	ld	e,(hl)
	push	hl
	ex	de,hl
	call	.Lcall_hl
	pop	hl
	jr	.Lctors

	; .init_array entries run from first to last.
.Linit_array:
	ld	hl,__init_array_start
.Linit_loop:
	ld	de,__init_array_end
	or	a
	sbc	hl,de
	add	hl,de
	jr	z,.Lmain
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	push	hl
	ex	de,hl
	call	.Lcall_hl
	pop	hl
	jr	.Linit_loop

.Lmain:
	call	main
	halt

.Lcall_hl:
	jp	(hl)
