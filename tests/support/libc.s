; Minimal libc for the test programs: the memory functions that GCC may
; call implicitly (struct copies, large initialisations).
;
; Calling convention: first argument in HL, second in DE, the remaining
; arguments on the stack (the first of them at sp+2 on entry).  Results in
; HL.  IY is free to use (it is never live across calls).

	.text

; void *memcpy (void *dest, const void *src, size_t n)
	.globl	memcpy
memcpy:
	push	hl		; return value
	ld	iy,4
	add	iy,sp
	ld	c,(iy+0)
	ld	b,(iy+1)	; BC = n
	ld	a,b
	or	c
	jr	z,.Lmemcpy_done
	ex	de,hl		; HL = src, DE = dest
	ldir
.Lmemcpy_done:
	pop	hl
	ret

; void *memmove (void *dest, const void *src, size_t n)
	.globl	memmove
memmove:
	push	hl		; return value
	ld	iy,4
	add	iy,sp
	ld	c,(iy+0)
	ld	b,(iy+1)	; BC = n
	ld	a,b
	or	c
	jr	z,.Lmemmove_done
	or	a
	sbc	hl,de		; flags for dest - src
	add	hl,de
	jr	c,.Lmemmove_fwd	; dest < src: copy forwards
	jr	z,.Lmemmove_done
	add	hl,bc		; dest > src: copy backwards from the end
	dec	hl
	ex	de,hl		; DE = dest + n - 1
	add	hl,bc
	dec	hl		; HL = src + n - 1
	lddr
	jr	.Lmemmove_done
.Lmemmove_fwd:
	ex	de,hl		; HL = src, DE = dest
	ldir
.Lmemmove_done:
	pop	hl
	ret

; void *memset (void *s, int c, size_t n)
	.globl	memset
memset:
	push	hl		; return value
	ld	iy,4
	add	iy,sp
	ld	c,(iy+0)
	ld	b,(iy+1)	; BC = n
	ld	a,b
	or	c
	jr	z,.Lmemset_done
	ld	(hl),e		; first byte
	dec	bc
	ld	a,b
	or	c
	jr	z,.Lmemset_done
	ld	d,h		; then copy it along
	ld	e,l
	inc	de
	ldir
.Lmemset_done:
	pop	hl
	ret

; int memcmp (const void *a, const void *b, size_t n)
	.globl	memcmp
memcmp:
	ld	iy,2
	add	iy,sp
	ld	c,(iy+0)
	ld	b,(iy+1)	; BC = n
.Lmemcmp_loop:
	ld	a,b
	or	c
	jr	z,.Lmemcmp_equal
	ld	a,(de)
	cp	(hl)		; b[i] - a[i]
	jr	nz,.Lmemcmp_diff
	inc	hl
	inc	de
	dec	bc
	jr	.Lmemcmp_loop
.Lmemcmp_diff:
	ld	hl,1		; carry: b[i] < a[i], so a > b
	ret	c
	ld	hl,-1
	ret
.Lmemcmp_equal:
	ld	hl,0
	ret
