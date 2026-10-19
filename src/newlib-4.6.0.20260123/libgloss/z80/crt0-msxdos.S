; Start-up code for MSX-DOS (and CP/M 2.2) .com programs.
;
; A .com program is loaded at 0x100 and entered there.  The word at 0x0006
; (the address of the BDOS entry point) is the top of the usable memory
; (TPA), so the stack starts there.
;
; Clears .bss, runs the constructors (.init_array), builds argc/argv from
; the command line at 0x80, calls main (argc, argv) and passes its result
; to __msxdos_exit (), which runs atexit handlers, flushes stdio and
; terminates through the BDOS.
;
; C symbols get a "_" prefix in assembly: main is _main, exit is _exit.

	.section .init,"ax"
	.globl	_start
_start:
	ld	hl,(6)		; top of the TPA
	ld	sp,hl

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
	call	___msxdos_args	; HL = argc, ___msxdos_argv = argv
	ld	de,(___msxdos_argv)
	call	_main
	jp	___msxdos_exit	; exit (main's result), see msxdos-exit.c

.Lcall_hl:
	jp	(hl)
