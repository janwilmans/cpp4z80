; Default start-up code for z80-elf programs.
; Copyright (C) 2026 Free Software Foundation, Inc.
;
; This file is part of GCC.
;
; GCC is free software; you can redistribute it and/or modify it under
; the terms of the GNU General Public License as published by the Free
; Software Foundation; either version 3, or (at your option) any later
; version.
;
; The code is placed in the .init section, which the default linker
; script puts at the very start of the program (0x100), so that the
; program can be entered at its load address.
;
;   - SP is set to __stack (define it with --defsym or in a linker script;
;     by default 0, i.e. the stack grows down from the top of memory).
;   - .bss is cleared and the constructors in .init_array are run.
;   - main () is called; its return value is passed to _exit, which by
;     default executes HALT (with the value still in HL).

	.section .init,"ax"
	.globl	_start
	.weak	__stack
_start:
	ld	sp,__stack

	; Clear .bss: __bss_start up to _end.
	ld	hl,_end
	ld	de,__bss_start
	or	a
	sbc	hl,de
	jr	z,.Lbss_done
	ld	b,h
	ld	c,l		; BC = size
	ld	h,d
	ld	l,e
	ld	(hl),0
	dec	bc
	ld	a,b
	or	c
	jr	z,.Lbss_done
	inc	de
	ldir
.Lbss_done:

	; Run the constructors, first to last.
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
	jp	_exit

.Lcall_hl:
	jp	(hl)

; Default _exit: stop the CPU, leaving the exit status in HL.
	.text
	.weak	_exit
_exit:
	halt
	jr	_exit
