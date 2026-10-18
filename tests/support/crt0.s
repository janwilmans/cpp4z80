; Minimal start-up code for the test programs.
;
; Sets up the stack at the top of memory, calls main() and halts.
; The test harness reads main's return value from HL after the HALT.

	.section .text.crt0,"ax"
	.globl	_start
_start:
	ld	sp,0
	call	main
	halt
