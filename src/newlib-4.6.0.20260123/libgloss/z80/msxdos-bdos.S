; BDOS interface for MSX-DOS (and CP/M 2.2) programs.
;
; C symbols get a "_" prefix in assembly.

	.text

; int __msxdos_bdos (int function, int de)
;
; Calls the BDOS function FUNCTION (in C) with DE; returns A.  The BDOS
; may change any register, including IX (the frame pointer) and IY.
	.globl	___msxdos_bdos
___msxdos_bdos:
	push	ix
	push	iy
	ld	c,l
	call	5
	pop	iy
	pop	ix
	ld	l,a
	ld	h,0
	ret

; void _exit (int status)
;
; MSX-DOS 2: function 0x62 (_TERM) ends the program with error code B.
; MSX-DOS 1 / CP/M do not have it (it returns), so fall back to function 0
; (system reset).
	.globl	__exit
__exit:
	ld	b,l
	ld	c,0x62
	call	5
	ld	c,0
	jp	5
